#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VIRTIO_MMIO_MAGIC_VALUE 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW 0x090
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH 0x0a4
#define VIRTIO_MMIO_CONFIG_GENERATION 0x0fc
#define VIRTIO_MMIO_CONFIG 0x100

#define VIRTIO_MMIO_MAGIC 0x74726976
#define VIRTIO_MMIO_VERSION_MODERN 0x02

// Device status bits.
#define VIRTIO_STATUS_ACKNOWLEDGE 0x01
#define VIRTIO_STATUS_DRIVER 0x02
#define VIRTIO_STATUS_DRIVER_OK 0x04
#define VIRTIO_STATUS_FEATURES_OK 0x08
#define VIRTIO_STATUS_NEEDS_RESET 0x40
#define VIRTIO_STATUS_FAILED 0x80

// Common feature bits.
#define VIRTIO_F_VERSION_1 32

// Virtqueue descriptor flags.
#define VIRTIO_DESC_F_NEXT (1 << 0)
#define VIRTIO_DESC_F_WRITE (1 << 1)

#define VIRTIO_DEVICE_ID_BLOCK 0x02

#define VIRTQUEUE_MAX_SIZE 256

typedef struct virtio_mmio_slot {
    uintptr_t addr;
    size_t size;

    struct virtio_mmio_slot *next;
} virtio_mmio_slot_t;

typedef struct [[gnu::packed]] {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtio_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTQUEUE_MAX_SIZE];
    uint16_t used_event;
} virtio_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} virtio_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    virtio_used_elem_t ring[VIRTQUEUE_MAX_SIZE];
    uint16_t avail_event;
} virtio_used_t;

typedef struct virtqueue {
    virtio_desc_t *desc;
    virtio_avail_t *avail;
    virtio_used_t *used;

    uint16_t size;
    uint16_t last_used;
} virtqueue_t;

extern virtio_mmio_slot_t *g_virtio_mmio_devices;

void virtio_mmio_register(uintptr_t addr, size_t size);

uint32_t virtio_mmio_read(virtio_mmio_slot_t *slot, size_t offset);
void virtio_mmio_write(virtio_mmio_slot_t *slot, size_t offset, uint32_t value);

uint32_t virtio_mmio_read_device_features(virtio_mmio_slot_t *slot, uint32_t selector);
void virtio_mmio_write_driver_features(virtio_mmio_slot_t *slot, uint32_t selector, uint32_t features);

uint8_t virtio_mmio_get_status(virtio_mmio_slot_t *slot);
void virtio_mmio_set_status(virtio_mmio_slot_t *slot, uint8_t status);
void virtio_mmio_reset(virtio_mmio_slot_t *slot);

uint64_t virtio_mmio_read_config_u64(virtio_mmio_slot_t *slot, size_t offset);

bool virtio_mmio_queue_setup(virtio_mmio_slot_t *slot, uint16_t index, virtqueue_t *queue);
void virtio_mmio_queue_notify(virtio_mmio_slot_t *slot, uint16_t index);

static inline uint64_t virtio_mmio_paddr(const void *ptr) {
    return (uint64_t) (uintptr_t) ptr;
}
