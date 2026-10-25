#define __USE_MINGW_ANSI_STDIO 1
#include "json_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_INITIAL_CAP 1024

static void ensure_capacity(json_builder_t *jb, size_t additional) {
    if (jb->length + additional + 64 >= jb->capacity) {
        size_t new_cap = jb->capacity * 2;
        if (new_cap < jb->length + additional + 64) {
            new_cap = jb->length + additional + 256;
        }
        char *new_buf = (char *)realloc(jb->buffer, new_cap);
        if (!new_buf) {
            fprintf(stderr, "json_utils: Out of memory reallocating json buffer\n");
            return;
        }
        jb->buffer = new_buf;
        jb->capacity = new_cap;
    }
}

static void append_raw(json_builder_t *jb, const char *str) {
    if (!str) return;
    size_t len = strlen(str);
    ensure_capacity(jb, len);
    memcpy(jb->buffer + jb->length, str, len);
    jb->length += len;
    jb->buffer[jb->length] = '\0';
}

static void append_escaped_string(json_builder_t *jb, const char *str) {
    if (!str) {
        append_raw(jb, "\"\"");
        return;
    }
    append_raw(jb, "\"");
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  append_raw(jb, "\\\""); break;
            case '\\': append_raw(jb, "\\\\"); break;
            case '\b': append_raw(jb, "\\b");  break;
            case '\f': append_raw(jb, "\\f");  break;
            case '\n': append_raw(jb, "\\n");  break;
            case '\r': append_raw(jb, "\\r");  break;
            case '\t': append_raw(jb, "\\t");  break;
            default:
                if ((unsigned char)*p < 32) {
                    char hex[8];
                    snprintf(hex, sizeof(hex), "\\u%04x", (unsigned char)*p);
                    append_raw(jb, hex);
                } else {
                    ensure_capacity(jb, 1);
                    jb->buffer[jb->length++] = *p;
                    jb->buffer[jb->length] = '\0';
                }
                break;
        }
    }
    append_raw(jb, "\"");
}

void json_init(json_builder_t *jb, size_t initial_capacity) {
    if (!jb) return;
    if (initial_capacity < 64) initial_capacity = JSON_INITIAL_CAP;
    jb->buffer = (char *)malloc(initial_capacity);
    if (jb->buffer) {
        jb->buffer[0] = '\0';
    }
    jb->length = 0;
    jb->capacity = initial_capacity;
    jb->need_comma = false;
}

void json_free(json_builder_t *jb) {
    if (!jb) return;
    if (jb->buffer) {
        free(jb->buffer);
        jb->buffer = NULL;
    }
    jb->length = 0;
    jb->capacity = 0;
    jb->need_comma = false;
}

void json_reset(json_builder_t *jb) {
    if (!jb) return;
    jb->length = 0;
    if (jb->buffer) {
        jb->buffer[0] = '\0';
    }
    jb->need_comma = false;
}

const char *json_get_string(const json_builder_t *jb) {
    if (!jb || !jb->buffer) return "{}";
    return jb->buffer;
}

void json_obj_start(json_builder_t *jb) {
    if (!jb) return;
    if (jb->need_comma) {
        append_raw(jb, ",");
    }
    append_raw(jb, "{");
    jb->need_comma = false;
}

void json_obj_end(json_builder_t *jb) {
    if (!jb) return;
    append_raw(jb, "}");
    jb->need_comma = true;
}

void json_arr_start(json_builder_t *jb) {
    if (!jb) return;
    if (jb->need_comma) {
        append_raw(jb, ",");
    }
    append_raw(jb, "[");
    jb->need_comma = false;
}

void json_arr_end(json_builder_t *jb) {
    if (!jb) return;
    append_raw(jb, "]");
    jb->need_comma = true;
}

void json_key(json_builder_t *jb, const char *key) {
    if (!jb || !key) return;
    if (jb->need_comma) {
        append_raw(jb, ",");
    }
    append_escaped_string(jb, key);
    append_raw(jb, ":");
    jb->need_comma = false;
}

void json_add_string(json_builder_t *jb, const char *key, const char *value) {
    if (!jb || !key) return;
    json_key(jb, key);
    append_escaped_string(jb, value ? value : "");
    jb->need_comma = true;
}

void json_add_int(json_builder_t *jb, const char *key, int64_t value) {
    if (!jb || !key) return;
    json_key(jb, key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)value);
    append_raw(jb, buf);
    jb->need_comma = true;
}

void json_add_uint64(json_builder_t *jb, const char *key, uint64_t value) {
    if (!jb || !key) return;
    json_key(jb, key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)value);
    append_raw(jb, buf);
    jb->need_comma = true;
}

void json_add_double(json_builder_t *jb, const char *key, double value, int precision) {
    if (!jb || !key) return;
    json_key(jb, key);
    char buf[64];
    char fmt[16];
    if (precision < 0) precision = 2;
    snprintf(fmt, sizeof(fmt), "%%.%df", precision);
    snprintf(buf, sizeof(buf), fmt, value);
    append_raw(jb, buf);
    jb->need_comma = true;
}

void json_add_bool(json_builder_t *jb, const char *key, bool value) {
    if (!jb || !key) return;
    json_key(jb, key);
    append_raw(jb, value ? "true" : "false");
    jb->need_comma = true;
}

void json_add_null(json_builder_t *jb, const char *key) {
    if (!jb || !key) return;
    json_key(jb, key);
    append_raw(jb, "null");
    jb->need_comma = true;
}

void json_arr_add_string(json_builder_t *jb, const char *value) {
    if (!jb) return;
    if (jb->need_comma) append_raw(jb, ",");
    append_escaped_string(jb, value ? value : "");
    jb->need_comma = true;
}

void json_arr_add_int(json_builder_t *jb, int64_t value) {
    if (!jb) return;
    if (jb->need_comma) append_raw(jb, ",");
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)value);
    append_raw(jb, buf);
    jb->need_comma = true;
}

void json_arr_add_double(json_builder_t *jb, double value, int precision) {
    if (!jb) return;
    if (jb->need_comma) append_raw(jb, ",");
    char buf[64];
    char fmt[16];
    if (precision < 0) precision = 2;
    snprintf(fmt, sizeof(fmt), "%%.%df", precision);
    snprintf(buf, sizeof(buf), fmt, value);
    append_raw(jb, buf);
    jb->need_comma = true;
}

void json_arr_add_raw(json_builder_t *jb, const char *raw_json) {
    if (!jb || !raw_json) return;
    if (jb->need_comma) append_raw(jb, ",");
    append_raw(jb, raw_json);
    jb->need_comma = true;
}

void json_add_raw(json_builder_t *jb, const char *key, const char *raw_json) {
    if (!jb || !raw_json) return;
    if (key) {
        json_key(jb, key);
        append_raw(jb, raw_json);
        jb->need_comma = true;
    } else {
        json_arr_add_raw(jb, raw_json);
    }
}

