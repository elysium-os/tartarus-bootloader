# Tartarus Protocol

## Machine State
The state the CPU shall be in at kernel entry.
### X86_64
- A20 line enabled
- General Registers:
    - `RAX` contains the kernel entry address
    - `RDI` contains a pointer to the boot information struct
    - `RBX`, `RCX`, `RDX`, `RSI`, `R8`, `R9`, `R10`, `R11`, `R12`, `R13`, `R14`, `R15`, are set to zero
    - `RBP` is set to zero
    - `RSP` is set to a stack that is at least 4KB in size
    - `RFLAGS.DF` clear
- Segment Registers:
    - `CS` is set to a flat 64bit ring0 code segment
    - `DS`, `SS`, `ES`, `FS`, `GS` are set to flat 64bit ring0 data segments
- Control Registers:
    - `CR0.PE` set
    - `CR0.PG` set
    - `CR0.WP` set
    - `CR4.PAE` set
    - `EFER.LMA` set
    - `EFER.NXE` set if available
- All register or bits of registers not described above are undefined

## Boot Information
A structure that contains information which is passed to the kernel, normally by passing a pointer via a register. This structure is defined in the `tartarus.h` header file. The boot information structure along with all the structures it refers are located within the HHDM.
## Virtual Memory
- 4GB and all non BAD memory entries will be mapped starting at
    - `0x0000 0000 0000 0000` *Identity mapped*
    - `0xFFFF 8000 0000 0000` *Higher Half Direct Memory (HHDM)*
- Kernel is mapped according to the ELF

## Physical Memory Map
- The first page will not be included in any entry
- All entries are ensured to not be overlapping
- All entries are ensured to be ordered by base
- Contigous memory of the same type is ensured to be one entry (No back to back entries of the same type)
- Entries of the following types will be paged aligned:
    - `USABLE`
    - `BOOT_RECLAIMABLE`
- `BOOT_RECLAIMABLE` entries should only be reclaimed once the kernel is either done with data provided by the bootloader or has moved it