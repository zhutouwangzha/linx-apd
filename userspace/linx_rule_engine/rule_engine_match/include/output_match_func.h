#ifndef __OUTPUT_MATCH_FUNC_H__
#define __OUTPUT_MATCH_FUNC_H__

#include "output_match_struct.h"

int linx_output_match_compile(linx_output_match_t **match, char *format);

int linx_output_match_format(linx_output_match_t *match, char *buffer, size_t buffer_size);

#endif /* __OUTPUT_MATCH_FUNC_H__ */
