#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"


typedef enum {
    mutable_df,
    hide_size_df,
    verbosity_df,
    indent_chars_df,
    dedent_chars_df,
    DF_COUNT
} defaults;

extern const char *DEFAULTS[DF_COUNT]; // = {

Config *initialize_config(void);

Config *load_config(const char *path);


#endif