#include "elf.h"

#include "arch/vm.h"
#include "common/log.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#define LITTLE_ENDIAN 1
#define CLASS64 2
#define MACHINE_386 0x3E
#define TYPE_EXECUTABLE 2

#define ELF_MAGIC "\x7f" "ELF"

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7

typedef uint64_t elf64_addr_t;
typedef uint64_t elf64_off_t;
typedef uint16_t elf64_half_t;
typedef uint32_t elf64_word_t;
typedef int32_t elf64_sword_t;
typedef uint64_t elf64_xword_t;
typedef int64_t elf64_sxword_t;

typedef struct [[gnu::packed]] {
    uint8_t magic[4];
    uint8_t class;
    uint8_t encoding;
    uint8_t file_version;
    uint8_t abi;
    uint8_t abi_version;
    uint8_t rsv0[6];
    uint8_t nident;
} elf_identifier_t;

typedef struct [[gnu::packed]] {
    elf_identifier_t identifier;
    elf64_half_t type;
    elf64_half_t machine;
    elf64_word_t version;
    elf64_addr_t entry;
    elf64_off_t program_header_offset;
    elf64_off_t section_header_offset;
    elf64_word_t flags;
    elf64_half_t header_size;
    elf64_half_t program_header_entry_size;
    elf64_half_t program_header_entry_count;
    elf64_half_t section_header_entry_size;
    elf64_half_t section_header_entry_count;
    elf64_half_t shstrtab_index;
} elf64_header_t;

typedef struct [[gnu::packed]] {
    elf64_word_t type;
    elf64_word_t flags;
    elf64_off_t offset;
    elf64_addr_t vaddr;
    elf64_addr_t sys_v_abi_undefined;
    elf64_xword_t filesz;
    elf64_xword_t memsz;
    elf64_xword_t alignment;
} elf64_program_header_t;

typedef struct [[gnu::packed]] {
    elf64_word_t name;
    elf64_word_t type;
    elf64_xword_t flags;
    elf64_addr_t addr;
    elf64_off_t offset;
    elf64_xword_t size;
    elf64_word_t link;
    elf64_word_t info;
    elf64_xword_t addralign;
    elf64_xword_t entsize;
} elf64_section_header_t;

static bool validate_elf(elf64_header_t *header) {
    if(memcmp(header->identifier.magic, ELF_MAGIC, 4) != 0) {
        log(LOG_LEVEL_ERROR, "elf: invalid identififer");
        return false;
    }

#ifdef __ARCH_X86_64
    if(header->machine != MACHINE_386) {
        log(LOG_LEVEL_ERROR, "elf: only the i386:x86-64 instruction-set is supported");
        return false;
    }

    if(header->identifier.encoding != LITTLE_ENDIAN) {
        log(LOG_LEVEL_ERROR, "elf: only little endian encoding is supported");
        return false;
    }
#endif

    if(header->identifier.class != CLASS64) {
        log(LOG_LEVEL_ERROR, "elf: only the 64bit class is supported");
        return false;
    }

    if(header->version > 1) {
        log(LOG_LEVEL_ERROR, "elf: unsupported version");
        return false;
    }
    if(header->type != TYPE_EXECUTABLE) {
        log(LOG_LEVEL_ERROR, "elf: only executables are supported");
        return false;
    }

    if(header->program_header_offset <= 0 && header->program_header_entry_count <= 0) {
        log(LOG_LEVEL_ERROR, "elf: no program headers. very suspicious");
        return false;
    }

    return true;
}

static bool read_header(vfs_node_t *file, elf64_header_t *header) {
    if(file->ops->read(file, header, 0, sizeof(elf64_header_t)) != sizeof(elf64_header_t)) {
        log(LOG_LEVEL_ERROR, "elf: unable to read header");
        return false;
    }
    return validate_elf(header);
}

elf_loaded_image_t *elf_load(vfs_node_t *file, void *address_space) {
    elf64_header_t header;
    if(!read_header(file, &header)) return NULL;

    elf64_addr_t lowest_vaddr = UINT64_MAX;
    elf64_addr_t highest_vaddr = 0;

    elf_region_t **regions = NULL;
    size_t region_count = 0;

    for(elf64_half_t i = 0; i < header.program_header_entry_count; i++) {
        elf64_program_header_t program_header;
        if(file->ops->read(file, &program_header, header.program_header_offset + header.program_header_entry_size * i, header.program_header_entry_size) != header.program_header_entry_size) {
            log(LOG_LEVEL_WARN, "elf: unable to read program header %u", i);
            return NULL;
        }
        if(program_header.type != PT_LOAD || program_header.memsz == 0) continue;
        if(program_header.vaddr < lowest_vaddr) lowest_vaddr = program_header.vaddr;
        if(program_header.vaddr + program_header.memsz > highest_vaddr) highest_vaddr = program_header.vaddr + program_header.memsz;

        elf_region_t *region = heap_alloc(sizeof(elf_region_t));
        region->vaddr = program_header.vaddr;
        region->size = program_header.memsz;
        region->read = (program_header.flags & VM_FLAG_READ) != 0;
        region->write = (program_header.flags & VM_FLAG_WRITE) != 0;
        region->execute = (program_header.flags & VM_FLAG_EXEC) != 0;

        regions = heap_realloc(regions, ++region_count * sizeof(elf_region_t *));
        regions[region_count - 1] = region;
    }

    elf64_xword_t size = highest_vaddr - lowest_vaddr;
    elf64_xword_t page_count = MATH_DIV_CEIL(size, PMM_GRANULARITY);
    void *paddr = pmm_alloc(PMM_AREA_STANDARD, page_count);
    for(elf64_half_t i = 0; i < header.program_header_entry_count; i++) {
        elf64_program_header_t program_header;
        if(file->ops->read(file, &program_header, header.program_header_offset + header.program_header_entry_size * i, header.program_header_entry_size) != header.program_header_entry_size) {
            log(LOG_LEVEL_WARN, "elf: unable to read program header %u", i);
            return NULL;
        }

        void *addr = paddr + (program_header.vaddr - lowest_vaddr);
        memset(addr, 0, program_header.memsz);

        elf64_addr_t aligned_vaddr = program_header.vaddr - program_header.vaddr % PMM_GRANULARITY;
        elf64_xword_t aligned_size = (program_header.memsz + (program_header.vaddr % PMM_GRANULARITY) + PMM_GRANULARITY - 1) / PMM_GRANULARITY * PMM_GRANULARITY;
        arch_vm_map(address_space, MATH_FLOOR((uintptr_t) addr, PMM_GRANULARITY), aligned_vaddr, aligned_size, program_header.flags & 0b111);

        if(program_header.type != PT_LOAD) continue;
        if(file->ops->read(file, addr, program_header.offset, program_header.filesz) != program_header.filesz) {
            pmm_free(paddr, page_count);
            log(LOG_LEVEL_WARN, "elf: unable to read program segment %u", i);
            return NULL;
        }
    }

    elf_loaded_image_t *image = heap_alloc(sizeof(elf_loaded_image_t));
    image->paddr = (uintptr_t) paddr;
    image->vaddr = lowest_vaddr;
    image->size = size;
    image->entry = header.entry;
    image->regions = regions;
    image->count = region_count;
    return image;
}

size_t elf_read_section(vfs_node_t *file, const char *section_name, void **data) {
    elf64_header_t header;
    if(!read_header(file, &header)) return 0;

    elf64_section_header_t shstrtab_header;
    if(file->ops->read(file, &shstrtab_header, header.section_header_offset + header.section_header_entry_size * header.shstrtab_index, header.section_header_entry_size) !=
       header.section_header_entry_size)
    {
        log(LOG_LEVEL_WARN, "elf: failed to read shstrtab header");
        return 0;
    }

    char *shstrtab = heap_alloc(shstrtab_header.size);
    if(file->ops->read(file, shstrtab, shstrtab_header.offset, shstrtab_header.size) != shstrtab_header.size) {
        log(LOG_LEVEL_WARN, "elf: failed to read shstrtab");
        return 0;
    }

    for(elf64_half_t i = 0; i < header.section_header_entry_count; i++) {
        elf64_section_header_t section_header;
        if(file->ops->read(file, &section_header, header.section_header_offset + header.section_header_entry_size * i, header.section_header_entry_size) != header.section_header_entry_size) {
            log(LOG_LEVEL_WARN, "elf: failed to read section header %u", i);
            return 0;
        }

        if(string_cmp(&shstrtab[section_header.name], section_name) != 0) continue;

        *data = heap_alloc(section_header.size);
        if(file->ops->read(file, *data, section_header.offset, section_header.size) != section_header.size) {
            log(LOG_LEVEL_WARN, "elf: failed to read section header `%s` data", shstrtab[section_header.name]);
            heap_free(*data);
            return 0;
        }
        heap_free(shstrtab);
        return section_header.size;
    }

    heap_free(shstrtab);
    return 0;
}
