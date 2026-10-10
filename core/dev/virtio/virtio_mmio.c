#include "virtio_mmio.h"

#include "common/log.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#if defined(__ARCH_X86_64)
#include "arch/x86_64/io.h"
#elif defined(__ARCH_AARCH64)
#include "arch/aarch64/io.h"
#elif defined(__ARCH_RISCV64)
#include "arch/riscv64/io.h"
#endif

virtio_mmio_slot_t *g_virtio_mmio_devices;

uint32_t virtio_mmio_read(virtio_mmio_slot_t *slot, size_t offset) {
    return arch_io_mem_read_u32(slot->addr + offset);
}

void virtio_mmio_write(virtio_mmio_slot_t *slot, size_t offset, uint32_t value) {
    arch_io_mem_write_u32(slot->addr + offset, value);
}

uint32_t virtio_mmio_read_device_features(virtio_mmio_slot_t *slot, uint32_t selector) {
    virtio_mmio_write(slot, VIRTIO_MMIO_DEVICE_FEATURES_SEL, selector);
    return virtio_mmio_read(slot, VIRTIO_MMIO_DEVICE_FEATURES);
}

void virtio_mmio_write_driver_features(virtio_mmio_slot_t *slot, uint32_t selector, uint32_t features) {
    virtio_mmio_write(slot, VIRTIO_MMIO_DRIVER_FEATURES_SEL, selector);
    virtio_mmio_write(slot, VIRTIO_MMIO_DRIVER_FEATURES, features);
}

uint8_t virtio_mmio_get_status(virtio_mmio_slot_t *slot) {
    return (uint8_t) virtio_mmio_read(slot, VIRTIO_MMIO_STATUS);
}

void virtio_mmio_set_status(virtio_mmio_slot_t *slot, uint8_t status) {
    virtio_mmio_write(slot, VIRTIO_MMIO_STATUS, status);
}

void virtio_mmio_reset(virtio_mmio_slot_t *slot) {
    virtio_mmio_write(slot, VIRTIO_MMIO_STATUS, 0);
    while(virtio_mmio_get_status(slot) != 0);
}

uint64_t virtio_mmio_read_config_u64(virtio_mmio_slot_t *slot, size_t offset) {
    uint64_t value;
    uint32_t generation;
    do {
        generation = virtio_mmio_read(slot, VIRTIO_MMIO_CONFIG_GENERATION);
        value = (uint64_t) virtio_mmio_read(slot, VIRTIO_MMIO_CONFIG + offset) | ((uint64_t) virtio_mmio_read(slot, VIRTIO_MMIO_CONFIG + offset + 4) << 32);
    } while(generation != virtio_mmio_read(slot, VIRTIO_MMIO_CONFIG_GENERATION));
    return value;
}

bool virtio_mmio_queue_setup(virtio_mmio_slot_t *slot, uint16_t index, virtqueue_t *queue) {
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_SEL, index);

    uint32_t max_size = virtio_mmio_read(slot, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if(max_size == 0) return false;

    uint16_t size = VIRTQUEUE_MAX_SIZE;
    while(size > max_size) size >>= 1;

    queue->size = size;
    queue->last_used = 0;

    queue->desc = pmm_alloc(PMM_AREA_STANDARD, 1);
    queue->avail = pmm_alloc(PMM_AREA_STANDARD, 1);
    queue->used = pmm_alloc(PMM_AREA_STANDARD, 1);
    memset(queue->desc, 0, PMM_GRANULARITY);
    memset(queue->avail, 0, PMM_GRANULARITY);
    memset(queue->used, 0, PMM_GRANULARITY);

    uint64_t desc = virtio_mmio_paddr(queue->desc);
    uint64_t avail = virtio_mmio_paddr(queue->avail);
    uint64_t used = virtio_mmio_paddr(queue->used);

    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_NUM, size);
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_DESC_LOW, (uint32_t) desc);
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_DESC_HIGH, (uint32_t) (desc >> 32));
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_DRIVER_LOW, (uint32_t) avail);
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_DRIVER_HIGH, (uint32_t) (avail >> 32));
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_DEVICE_LOW, (uint32_t) used);
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_DEVICE_HIGH, (uint32_t) (used >> 32));
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_READY, 1);
    return true;
}

void virtio_mmio_queue_notify(virtio_mmio_slot_t *slot, uint16_t index) {
    virtio_mmio_write(slot, VIRTIO_MMIO_QUEUE_NOTIFY, index);
}

void virtio_mmio_register(uintptr_t addr, size_t size) {
    virtio_mmio_slot_t *slot = heap_alloc(sizeof(virtio_mmio_slot_t));
    slot->addr = addr;
    slot->size = size;

    uint32_t magic = virtio_mmio_read(slot, VIRTIO_MMIO_MAGIC_VALUE);
    if(magic != VIRTIO_MMIO_MAGIC) {
        log(LOG_LEVEL_WARN, "rejecting virtio-mmio @ 0x%lx | invalid magic", addr);
        heap_free(slot);
        return;
    }

    uint32_t version = virtio_mmio_read(slot, VIRTIO_MMIO_VERSION);
    if(version != VIRTIO_MMIO_VERSION_MODERN) {
        log(LOG_LEVEL_WARN, "rejecting virtio-mmio @ 0x%lx | unsupported version %u (legacy is not supported)", addr, version);
        heap_free(slot);
        return;
    }

    uint32_t device_id = virtio_mmio_read(slot, VIRTIO_MMIO_DEVICE_ID);
    uint32_t vendor_id = virtio_mmio_read(slot, VIRTIO_MMIO_VENDOR_ID);

    if(device_id == 0x00) {
        log(LOG_LEVEL_WARN, "rejecting virtio-mmio @ 0x%lx | placeholder device", addr);
        heap_free(slot);
        return;
    }

    log(LOG_LEVEL_DEBUG, "registering virtio-mmio device @ 0x%lx, id=0x%04x:0x%04x", addr, vendor_id, device_id);

    slot->next = g_virtio_mmio_devices;
    g_virtio_mmio_devices = slot;
}
