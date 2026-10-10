#include "virtio_blk.h"

#include "common/log.h"
#include "lib/container.h"
#include "memory/heap.h"
#include "virtio_mmio.h"

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

// feature bits
#define VIRTIO_BLK_F_RO 5

// config offsets
#define VIRTIO_BLK_CONFIG_CAPACITY 0x00

#define VIRTIO_BLK_SECTOR_SIZE 512
#define VIRTIO_BLK_REQUEST_TIMEOUT 100000000

typedef struct [[gnu::packed]] {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} virtio_blk_req_t;

typedef struct {
    disk_t common;

    virtio_mmio_slot_t *slot;
    virtqueue_t queue;
} virtio_blk_disk_t;

#define VIRTIO_BLK(DISK) (CONTAINER_OF((DISK), virtio_blk_disk_t, common))

static uint32_t g_virtio_blk_next_id;

static bool virtio_blk_request(virtio_blk_disk_t *disk, uint32_t type, uint64_t sector, uint32_t length, void *buffer, bool device_writable) {
    virtqueue_t *queue = &disk->queue;
    virtio_mmio_slot_t *slot = disk->slot;

    virtio_blk_req_t request = {
        .type = type,
        .reserved = 0,
        .sector = sector,
    };
    volatile uint8_t status = 0xff;

    virtio_desc_t *desc = queue->desc;
    desc[0] = (virtio_desc_t) {
        .addr = virtio_mmio_paddr(&request),
        .len = sizeof(request),
        .flags = VIRTIO_DESC_F_NEXT,
        .next = 1,
    };
    desc[1] = (virtio_desc_t) {
        .addr = virtio_mmio_paddr(buffer),
        .len = length,
        .flags = device_writable ? (VIRTIO_DESC_F_NEXT | VIRTIO_DESC_F_WRITE) : VIRTIO_DESC_F_NEXT,
        .next = 2,
    };
    desc[2] = (virtio_desc_t) {
        .addr = virtio_mmio_paddr((const void *) &status),
        .len = sizeof(status),
        .flags = VIRTIO_DESC_F_WRITE,
        .next = 0,
    };

    uint16_t head = 0;
    uint16_t avail_index = __atomic_load_n(&queue->avail->idx, __ATOMIC_RELAXED);
    queue->avail->ring[avail_index % queue->size] = head;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    __atomic_store_n(&queue->avail->idx, avail_index + 1, __ATOMIC_RELEASE);

    virtio_mmio_queue_notify(slot, 0);

    uint64_t timeout = VIRTIO_BLK_REQUEST_TIMEOUT;
    while(__atomic_load_n(&queue->used->idx, __ATOMIC_ACQUIRE) == queue->last_used) {
        if(--timeout == 0) {
            log(LOG_LEVEL_ERROR, "virtio-blk: request timed out");
            return true;
        }
    }
    queue->last_used++;

    if(status != 0) {
        log(LOG_LEVEL_ERROR, "virtio-blk: request failed with status %u", status);
        return true;
    }
    return false;
}

bool virtio_blk_read(disk_t *common, uint64_t lba, uint64_t sector_count, void *dest) {
    virtio_blk_disk_t *disk = VIRTIO_BLK(common);
    return virtio_blk_request(disk, VIRTIO_BLK_T_IN, lba, (uint32_t) (sector_count * common->sector_size), dest, true);
}

bool virtio_blk_write(disk_t *common, uint64_t lba, uint64_t sector_count, void *src) {
    virtio_blk_disk_t *disk = VIRTIO_BLK(common);
    if(common->read_only) return true;
    return virtio_blk_request(disk, VIRTIO_BLK_T_OUT, lba, (uint32_t) (sector_count * common->sector_size), src, false);
}

static bool virtio_blk_probe(virtio_mmio_slot_t *slot) {
    virtio_mmio_reset(slot);

    uint8_t status = 0;
    virtio_mmio_set_status(slot, status |= VIRTIO_STATUS_ACKNOWLEDGE);
    virtio_mmio_set_status(slot, status |= VIRTIO_STATUS_DRIVER);

    uint32_t features_low = virtio_mmio_read_device_features(slot, 0);
    uint32_t features_high = virtio_mmio_read_device_features(slot, 1);

    if(!(features_high & (1u << (VIRTIO_F_VERSION_1 - 32)))) {
        log(LOG_LEVEL_WARN, "virtio-blk @ 0x%lx: VIRTIO_F_VERSION_1 not offered", slot->addr);
        virtio_mmio_set_status(slot, VIRTIO_STATUS_FAILED);
        return false;
    }

    bool read_only = (features_low & (1u << VIRTIO_BLK_F_RO)) != 0;

    virtio_mmio_write_driver_features(slot, 0, features_low & (1u << VIRTIO_BLK_F_RO));
    virtio_mmio_write_driver_features(slot, 1, 1u << (VIRTIO_F_VERSION_1 - 32));

    virtio_mmio_set_status(slot, status |= VIRTIO_STATUS_FEATURES_OK);
    if(!(virtio_mmio_get_status(slot) & VIRTIO_STATUS_FEATURES_OK)) {
        log(LOG_LEVEL_ERROR, "virtio-blk @ 0x%lx: device rejected our feature set", slot->addr);
        virtio_mmio_set_status(slot, VIRTIO_STATUS_FAILED);
        return false;
    }

    virtio_blk_disk_t *disk = heap_alloc(sizeof(virtio_blk_disk_t));
    if(!virtio_mmio_queue_setup(slot, 0, &disk->queue)) {
        log(LOG_LEVEL_ERROR, "virtio-blk @ 0x%lx: failed to set up request queue", slot->addr);
        heap_free(disk);
        virtio_mmio_set_status(slot, VIRTIO_STATUS_FAILED);
        return false;
    }

    uint64_t capacity = virtio_mmio_read_config_u64(slot, VIRTIO_BLK_CONFIG_CAPACITY);

    disk->slot = slot;
    disk->common.id = g_virtio_blk_next_id++;
    disk->common.read_only = read_only;
    disk->common.sector_size = VIRTIO_BLK_SECTOR_SIZE;
    disk->common.sector_count = capacity;
    disk->common.optimal_transfer_size = 1;
    disk->common.partitions = nullptr;

    virtio_mmio_set_status(slot, status |= VIRTIO_STATUS_DRIVER_OK);

    disk_initialize_partitions(&disk->common);

    disk->common.next = g_disks;
    g_disks = &disk->common;

    log(LOG_LEVEL_INFO, "virtio-blk: drive %u: %lu sectors (%lu KiB)%s", disk->common.id, capacity, (capacity * VIRTIO_BLK_SECTOR_SIZE) / 1024, read_only ? " [read-only]" : "");
    return true;
}

void virtio_blk_initialize(void) {
    for(virtio_mmio_slot_t *slot = g_virtio_mmio_devices; slot != nullptr; slot = slot->next) {
        if(virtio_mmio_read(slot, VIRTIO_MMIO_DEVICE_ID) != VIRTIO_DEVICE_ID_BLOCK) continue;
        virtio_blk_probe(slot);
    }
}
