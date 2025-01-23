#include "common/log.h"
#include "memory/pmm.h"

#include <stddef.h>

#define VERSION_MAJOR 4
#define VERSION_MINOR 0

#ifdef __ENV_DEVELOPMENT
#define VERSION_TAG " (development)"
#else
#define VERSION_TAG ""
#endif

[[noreturn]] void core() {
    log(LOG_LEVEL_INFO, "Tartarus v%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_TAG);

    for(size_t i = 0; i < g_pmm_map_size; i++) {
        log(LOG_LEVEL_DEBUG, "PMM_MAP[%lu] = { base: %#llx, length: %#llx, type: %u }", i, g_pmm_map[i].base, g_pmm_map[i].length, g_pmm_map[i].type);
    }
    log(LOG_LEVEL_INFO, "Loaded physical memory map (%lu entries)", g_pmm_map_size);

    for(;;);
}
