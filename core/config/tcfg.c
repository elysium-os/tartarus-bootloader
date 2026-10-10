#include "tcfg.h"

#include "lib/string.h"
#include "memory/heap.h"

void tcfg_free_object(tcfg_object_t *object) {
    for(size_t i = 0; i < object->field_count; i++) {
        heap_free(object->field_keys[i]);
        tcfg_free_value(object->field_values[i]);
    }
    heap_free(object->field_keys);
    heap_free(object->field_values);
    heap_free(object);
}

void tcfg_free_array(tcfg_array_t *array) {
    for(size_t i = 0; i < array->length; i++) tcfg_free_value(array->items[i]);
    heap_free(array->items);
    heap_free(array);
}

void tcfg_free_value(tcfg_value_t *value) {
    switch(value->type) {
        case TCFG_VALUE_TYPE_OBJECT:  tcfg_free_object(value->object); break;
        case TCFG_VALUE_TYPE_ARRAY:   tcfg_free_array(value->array); break;
        case TCFG_VALUE_TYPE_STRING:  heap_free(value->string); break;
        case TCFG_VALUE_TYPE_INTEGER: break;
        case TCFG_VALUE_TYPE_BOOLEAN: break;
    }
    heap_free(value);
}

tcfg_value_t *tcfg_object_get_field(tcfg_object_t *object, const char *field_name) {
    for(size_t i = 0; i < object->field_count; i++) {
        if(string_cmp(object->field_keys[i], field_name) == 0) return object->field_values[i];
    }
    return nullptr;
}
