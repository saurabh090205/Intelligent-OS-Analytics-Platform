#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
    bool need_comma;
} json_builder_t;

void json_init(json_builder_t *jb, size_t initial_capacity);
void json_free(json_builder_t *jb);
void json_reset(json_builder_t *jb);
const char *json_get_string(const json_builder_t *jb);

void json_obj_start(json_builder_t *jb);
void json_obj_end(json_builder_t *jb);

void json_arr_start(json_builder_t *jb);
void json_arr_end(json_builder_t *jb);

void json_key(json_builder_t *jb, const char *key);

void json_add_string(json_builder_t *jb, const char *key, const char *value);
void json_add_int(json_builder_t *jb, const char *key, int64_t value);
void json_add_uint64(json_builder_t *jb, const char *key, uint64_t value);
void json_add_double(json_builder_t *jb, const char *key, double value, int precision);
void json_add_bool(json_builder_t *jb, const char *key, bool value);
void json_add_null(json_builder_t *jb, const char *key);

/* Array elements without keys */
void json_arr_add_string(json_builder_t *jb, const char *value);
void json_arr_add_int(json_builder_t *jb, int64_t value);
void json_arr_add_double(json_builder_t *jb, double value, int precision);

/* Raw JSON injection */
void json_add_raw(json_builder_t *jb, const char *key, const char *raw_json);
void json_arr_add_raw(json_builder_t *jb, const char *raw_json);

#endif /* JSON_UTILS_H */
