#include "protocol.h"

#include "lib/string.h"

[[noreturn]] void protocol_linux(config_t *config, vfs_node_t *kernel_node, fb_t *fb);
[[noreturn]] void protocol_tartarus(config_t *config, vfs_node_t *kernel_node, fb_t *fb);

static protocol_t protocols[] = {
    (protocol_t) {.name = "tartarus", .entry = protocol_tartarus},
    (protocol_t) {.name = "linux",    .entry = protocol_linux   },
};

protocol_t *protocol_match(const char *name) {
    for(size_t i = 0; i < sizeof(protocols) / sizeof(protocol_t); i++) {
        if(!string_eq(protocols[i].name, name)) continue;
        return &protocols[i];
    }
    return NULL;
}
