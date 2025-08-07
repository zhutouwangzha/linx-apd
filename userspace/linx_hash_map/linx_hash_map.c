#include <stddef.h>

#include "linx_hash_map.h"
#include "linx_event_rich.h"
#include "linx_event_table.h"

static linx_hash_map_t *s_linx_hash_map = NULL;

static void destroy_field_info(field_info_t *fields)
{
    field_info_t *current, *tmp;

    if (!fields) {
        return;
    }

    HASH_ITER(hh, fields, current, tmp) {
        HASH_DEL(fields, current);
        free(current);
    }
}

static void destroy_field_table(field_table_t *table)
{
    if (!table) {
        return;
    }

    destroy_field_info(table->fields);
    free(table->table_name);
    free(table);
}

int linx_hash_map_init(void)
{
    if (s_linx_hash_map) {
        return -1;
    }

    s_linx_hash_map = (linx_hash_map_t *)malloc(sizeof(linx_hash_map_t));
    if (s_linx_hash_map == NULL) {
        return -1;
    }

    s_linx_hash_map->tables = NULL;

    return 0;
}

void linx_hash_map_deinit(void)
{
    field_table_t *current_table, *tmp_table;

    if (s_linx_hash_map == NULL) {
        return;
    }

    HASH_ITER(hh, s_linx_hash_map->tables, current_table, tmp_table) {
        HASH_DEL(s_linx_hash_map->tables, current_table);
        destroy_field_table(current_table);
    }

    free(s_linx_hash_map);
    s_linx_hash_map = NULL;
}

int linx_hash_map_create_table(const char *table_name, void *base_addr)
{
    field_table_t *existing_table, *new_table;

    if (s_linx_hash_map == NULL || table_name == NULL) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, existing_table);
    if (existing_table) {
        return -1;
    }

    new_table = malloc(sizeof(field_table_t));
    if (new_table == NULL) {
        return -1;
    }

    new_table->table_name = strdup(table_name);
    new_table->base_addr = base_addr;   /* 可以为NULL,表示延迟绑定 */
    new_table->fields = NULL;

    HASH_ADD_STR(s_linx_hash_map->tables, table_name, new_table);

    return 0;
}

int linx_hash_map_remove_table(const char *table_name)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;
    }

    HASH_DEL(s_linx_hash_map->tables, table);
    s_linx_hash_map->size--;

    return 0;
}

int linx_hash_map_add_field(const char *table_name, const char *field_name, size_t offset, size_t size, linx_field_type_t type)
{
    field_table_t *table;
    field_info_t *existing_field, *new_field;

    if (!s_linx_hash_map || !table_name || !field_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;
    }

    HASH_FIND_STR(table->fields, field_name, existing_field);
    if (existing_field != NULL) {
        return -1;
    }

    new_field = malloc(sizeof(field_info_t));
    if (new_field == NULL) {
        return -1;
    }

    new_field->key = (char *)field_name;
    new_field->offset = offset;
    new_field->type = type;
    new_field->size = size;

    HASH_ADD_STR(table->fields, key, new_field);

    return 0;
}

int linx_hash_map_add_field_batch(const char *table_name, const field_mapping_t *mappings, size_t count)
{
    int ret;

    if (!s_linx_hash_map || !table_name || !mappings) {
        return -1;
    }

    ret = linx_hash_map_create_table(table_name, NULL);
    if (ret) {
        return ret;
    }

    for (size_t i = 0; i < count; i++) {
        ret = linx_hash_map_add_field(table_name, mappings[i].field_name, mappings[i].offset, mappings[i].size, mappings[i].type);
        if (ret) {
            return ret;
        }
    }

    return ret;
}

field_result_t linx_hash_map_get_field(const char *table_name, const char *field_name)
{
    field_table_t *table;
    field_info_t *field;
    field_result_t result = {0};

    result.found = false;

    if (!s_linx_hash_map || !table_name || !field_name) {
        return result;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return result;
    }

    HASH_FIND_STR(table->fields, field_name, field);
    if (!field) {
        return result;
    }

    result.offset = field->offset;
    result.type = field->type;
    result.size = field->size;
    result.found = true;
    result.table_name = table->table_name;
    result.field_name = field->key;

    return result;
}

field_result_t linx_hash_map_get_field_by_path(char *path)
{
    char *table_name, *field_name, *arg;
    field_result_t result = {0};
    result.found = false;

    if (path == NULL) {
        return  result;
    }

    table_name = strtok(path, ".");
    if (table_name == NULL) {
        return result;
    }

    field_name = strtok(NULL, ".");
    if (field_name == NULL) {
        return result;
    }

    result = linx_hash_map_get_field(table_name, field_name);

    arg = strtok(NULL, ".");
    if (arg == NULL) {
        result.arg = NULL;
    } else {
        result.arg = strdup(arg);
    }

    result.event_type = &(linx_event_rich_get()->num);

    return result;
}

void *linx_hash_map_get_value_ptr(const field_result_t *field, linx_field_type_t *type)
{
    void *base_addr, *ptr;

    if (!field->found) {
        return NULL;
    }

    base_addr = linx_hash_map_get_table_base(field->table_name);
    if (base_addr == NULL) {
        return NULL;
    }

    if (field->arg) {
        char *endptr;
        long index = strtol(field->arg, &endptr, 10);

        /* 如果不是纯数字，则通过名称查找下标 */
        if (*endptr != '\0') {
            for (index = 0; index < g_linx_event_table[*field->event_type].nparams; index++) {
                if (strcmp(g_linx_event_table[*field->event_type].params[index].name, field->arg) == 0) {
                    break;
                }
            }
        }

        if (index >= g_linx_event_table[*field->event_type].nparams) {
            return NULL;
        }

        ptr = (void *)((char *)base_addr + field->offset + index * sizeof(void *));
        *type = g_linx_event_table[*field->event_type].params[index].type;
    } else {
        ptr = (void *)((char *)base_addr + field->offset);
        *type = field->type;
    }

    return ptr;
}

int linx_hash_map_update_table_base(const char *table_name, void *base_addr)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return -1;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return -1;
    }

    table->base_addr = base_addr;

    return 0;
}

int linx_hash_map_update_tables_base(field_update_table_t *tables, size_t num_tables)
{
    int ret = 0;

    if (!s_linx_hash_map || !tables) {
        return -1;
    }

    for (size_t i = 0; i < num_tables; i++) {
        ret = linx_hash_map_update_table_base(tables[i].table_name, tables[i].base_addr);
        if (ret) {
            return ret;
        }
    }

    return ret;
}

void *linx_hash_map_get_table_base(const char *table_name)
{
    field_table_t *table;

    if (!s_linx_hash_map || !table_name) {
        return NULL;
    }

    HASH_FIND_STR(s_linx_hash_map->tables, table_name, table);
    if (!table) {
        return NULL;
    }

    return table->base_addr;
}

int linx_hash_map_list_tables(char ***table_names, size_t *num_tables)
{
    field_table_t *current, *tmp;
    char **names;
    size_t table_count = 0;
    size_t index = 0;

    if (!s_linx_hash_map || !table_names || !num_tables) {
        return -1;
    }

    HASH_ITER(hh, s_linx_hash_map->tables, current, tmp) {
        table_count++;
    }

    if (table_count == 0) {
        *table_names = NULL;
        *num_tables = 0;
        return 0;
    }

    names = malloc(sizeof(char *) * table_count);
    if (!names) {
        return -1;
    }

    HASH_ITER(hh, s_linx_hash_map->tables, current, tmp) {
        names[index] = strdup(current->table_name);
        if (!names[index]) {
            for (size_t i = 0; i < index; i++) {
                free(names[i]);
                names[i] = NULL;
            }

            free(names);
            names = NULL;
            return -1;
        }

        index++;
    }

    *table_names = names;
    *num_tables = table_count;
    return 0;
}

void linx_hash_map_free_table_list(char **table_names, size_t num_tables)
{
    if (!table_names) {
        return;
    }

    for (size_t i = 0; i < num_tables; i++) {
        free(table_names[i]);
        table_names[i] = NULL;
    }

    free(table_names);
    table_names = NULL;
}
