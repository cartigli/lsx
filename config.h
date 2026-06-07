#ifndef CONFIG_H
#define CONFIG_H

// runtime mode
typedef enum { MENU_MODE, EDIT_MODE } MODE;

typedef enum { mutable_df, hide_size_df, verbosity_df, DF_COUNT } defaults;
extern const char *DEFAULTS[DF_COUNT];

// runtime config
typedef struct {
    char *root;
    MODE mode;

    int mutable;
    int hide_size;
    int verbosity;
    int indent_len;

    int colors_loaded;

    const char *indent_chars;
    const char *dedent_chars;
} Config;

void initialize_config(Config *config);

#endif
