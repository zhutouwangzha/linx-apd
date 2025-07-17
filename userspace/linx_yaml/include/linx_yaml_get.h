#ifndef __LINX_YAML_GET_H__
#define __LINX_YAML_GET_H__ 

#include "linx_yaml_node.h"

linx_yaml_node_t *linx_yaml_get_node_by_path(linx_yaml_node_t *root, const char *path);

const char *linx_yaml_get_string(linx_yaml_node_t *root, const char *path, const char *default_value);

int linx_yaml_get_int(linx_yaml_node_t *root, const char *path, int default_value);

int linx_yaml_get_bool(linx_yaml_node_t *root, const char *path, int default_value);

int linx_yaml_get_sequence_length(linx_yaml_node_t *root, const char *path);

#endif /* __LINX_YAML_GET_H__ */