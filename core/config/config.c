#include "config.h"

#include "common/log.h"
#include "common/panic.h"
#include "config/lexer.h"
#include "config/parse.h"
#include "config/tcfg.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "protocol/protocol.h"

#include <stddef.h>

#define STATUS_CONTEXT_BEFORE 3

config_t g_config;
size_t g_config_selected_entry = 0;

static size_t line_end_from(const char *data, size_t data_length, size_t line_start) {
    size_t line_end = line_start;
    while(line_end < data_length && data[line_end] != '\n') line_end++;
    return line_end;
}

static size_t prev_line_start(const char *data, size_t line_start) {
    if(line_start == 0) return 0;
    size_t cursor = line_start - 1;
    while(cursor > 0 && data[cursor - 1] != '\n') cursor--;
    return cursor;
}

static void print_source_line(const char *data, size_t line_start, size_t line_end, size_t line_no, int gutter_width) {
    log(LOG_LEVEL_ERROR, "%*zu | %.*s", gutter_width, line_no, (int) (line_end - line_start), data + line_start);
}

static void print_status(const char *data, size_t data_length, parse_status_t status) {
    size_t start = status.start > data_length ? data_length : status.start;
    size_t end = status.end > data_length ? data_length : status.end;

    size_t line_no = 1, line_start = 0;
    for(size_t i = 0; i < start; i++) {
        if(data[i] == '\n') {
            line_no++;
            line_start = i + 1;
        }
    }
    size_t line_end = line_end_from(data, data_length, line_start);

    if(end > line_end) end = line_end;
    size_t col = start - line_start;
    size_t width = end > start ? end - start : 1;

    size_t first_line_start = line_start;
    size_t first_line_no = line_no;
    for(size_t i = 0; i < STATUS_CONTEXT_BEFORE && first_line_start > 0; i++) {
        first_line_start = prev_line_start(data, first_line_start);
        first_line_no--;
    }

    int gutter_width = 1;
    for(size_t n = line_no; n >= 10; n /= 10) gutter_width++;

    log(LOG_LEVEL_ERROR, "error: %s (line %zu, col %zu)", status.issue, line_no, col + 1);

    size_t cursor = first_line_start;
    for(size_t no = first_line_no; no < line_no; no++) {
        size_t end_of_line = line_end_from(data, data_length, cursor);
        print_source_line(data, cursor, end_of_line, no, gutter_width);
        cursor = end_of_line + 1;
    }

    print_source_line(data, line_start, line_end, line_no, gutter_width);

    size_t indent = (size_t) gutter_width + 3 + col;
    char *marker = heap_alloc(indent + width + 1);
    memset(marker, ' ', indent);
    memset(marker + indent, '^', width);
    marker[indent + width] = '\0';
    log(LOG_LEVEL_ERROR, "%s", marker);
    heap_free(marker);
}

void config_load(const char *data, size_t data_length) {
    lexer_t lexer = lexer_new(data, data_length);

    tcfg_object_t *root;
    parse_status_t status = parse_root(&lexer, &root);
    if(status.issue != nullptr) {
        print_status(data, data_length, status);
        panic("Failed to parse config");
    }

    tcfg_value_t *boot_entries = tcfg_object_get_field(root, "boot_entries");
    if(boot_entries == nullptr) panic("Config Error: Missing entry `boot_entries`");
    if(boot_entries->type != TCFG_VALUE_TYPE_OBJECT) panic("Config Error: Invalid value for `boot_entries`");
    if(boot_entries->object->field_count == 0) panic("Config Error: Zero boot entries defined");

    config_t config = {
        .boot_entries = heap_alloc(boot_entries->object->field_count * sizeof(config_boot_entry_t)),
        .boot_entry_count = boot_entries->object->field_count,
        .framebuffer_height = 1080,
        .framebuffer_width = 1920,
        .framebuffer_strict_rgb = false
    };

    tcfg_value_t *fb = tcfg_object_get_field(root, "framebuffer");
    if(fb != nullptr) {
        if(fb->type != TCFG_VALUE_TYPE_OBJECT) panic("Config Error: Invalid value for `framebuffer`");

        tcfg_value_t *strict_rgb = tcfg_object_get_field(fb->object, "strict_rgb");
        if(strict_rgb != nullptr) {
            if(strict_rgb->type != TCFG_VALUE_TYPE_BOOLEAN) panic("Config Error: Invalid value for `framebuffer.strict_rgb`");
            config.framebuffer_strict_rgb = strict_rgb;
        }

        tcfg_value_t *width = tcfg_object_get_field(fb->object, "width");
        if(width != nullptr) {
            if(width->type != TCFG_VALUE_TYPE_INTEGER) panic("Config Error: Invalid value for `framebuffer.width`");
            if(width->integer <= 0) panic("Config Error: Integer must be positive for `framebuffer.width`");
            config.framebuffer_width = width->integer;
        }

        tcfg_value_t *height = tcfg_object_get_field(fb->object, "height");
        if(height != nullptr) {
            if(height->type != TCFG_VALUE_TYPE_INTEGER) panic("Config Error: Invalid value for `framebuffer.height`");
            if(height->integer <= 0) panic("Config Error: Integer must be positive for `framebuffer.height`");
            config.framebuffer_height = height->integer;
        }
    }

    for(size_t i = 0; i < boot_entries->object->field_count; i++) {
        config_boot_entry_t *entry = &config.boot_entries[i];

        const char *name = boot_entries->object->field_keys[i];
        tcfg_value_t *boot_entry_value = boot_entries->object->field_values[i];
        if(boot_entry_value->type != TCFG_VALUE_TYPE_OBJECT) panic("Config Error: Invalid value for `boot_entries.%s`", name);

        tcfg_value_t *kernel_value = tcfg_object_get_field(boot_entry_value->object, "kernel");
        if(kernel_value == nullptr) panic("Config Error: Missing entry `boot_entries.%s.kernel`", name);
        if(kernel_value->type != TCFG_VALUE_TYPE_STRING) panic("Config Error: Invalid value for `boot_entries.%s.kernel`", name);

        entry->kernel = heap_strdup(kernel_value->string);

        tcfg_value_t *protocol_value = tcfg_object_get_field(boot_entry_value->object, "protocol");
        if(protocol_value == nullptr) panic("Config Error: Missing entry `boot_entries.%s.protocol`", name);
        if(protocol_value->type != TCFG_VALUE_TYPE_STRING) panic("Config Error: Invalid value for `boot_entries.%s.protocol`", name);

        if(string_case_eq(protocol_value->string, "tartarus")) {
            entry->protocol = PROTOCOL_TARTARUS;
        } else {
            panic("Config Error: Unknown protocol for boot entry `%s`", name);
        }

        switch(entry->protocol) {
            case PROTOCOL_TARTARUS:
                entry->protocol_tartarus.enable_smp = true;

                tcfg_value_t *smp_value = tcfg_object_get_field(boot_entry_value->object, "smp");
                if(smp_value != nullptr) {
                    if(smp_value->type != TCFG_VALUE_TYPE_BOOLEAN) panic("Config Error: Invalid value for `boot_entries.%s.smp`", name);
                    entry->protocol_tartarus.enable_smp = smp_value->boolean;
                }

                size_t module_count = 0;
                char **modules = nullptr;

                tcfg_value_t *module_values = tcfg_object_get_field(boot_entry_value->object, "modules");
                if(module_values != nullptr) {
                    if(module_values->type != TCFG_VALUE_TYPE_ARRAY) panic("Config Error: Invalid value for `boot_entries.%s.modules`", name);

                    module_count = module_values->array->length;
                    modules = heap_alloc(module_count * sizeof(const char *));
                    for(size_t j = 0; j < module_values->array->length; j++) {
                        tcfg_value_t *module_value = module_values->array->items[j];
                        if(module_value->type != TCFG_VALUE_TYPE_STRING) panic("Config Error: Invalid value for `boot_entries.%s.modules[%lu]`", name, j);

                        modules[j] = heap_strdup(module_value->string);
                    }
                }

                entry->protocol_tartarus.module_count = module_count;
                entry->protocol_tartarus.module_paths = modules;
                break;
        }
    }

    tcfg_free_object(root);

    g_config = config;
}
