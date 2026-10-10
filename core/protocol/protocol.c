#include "protocol.h"

#include "lib/string.h"

[[noreturn]] void protocol_tartarus(config_t *config, vfs_node_t *kernel_node, fb_t *fb);

#ifdef __ARCH_X86_64
[[noreturn]] void protocol_linux(config_t *config, vfs_node_t *kernel_node, fb_t *fb);
#endif

static protocol_t protocols[] = {
    (protocol_t) {.name = "tartarus", .entry = protocol_tartarus},
#ifdef __ARCH_X86_64
    (protocol_t) {.name = "linux",    .entry = protocol_linux   },
#endif
};

protocol_t *protocol_match(const char *name) {
    for(size_t i = 0; i < sizeof(protocols) / sizeof(protocol_t); i++) {
        if(!string_eq(protocols[i].name, name)) continue;
        return &protocols[i];
    }
    return nullptr;
}
