/**
 * @file yaml_loader.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <cyaml/cyaml.h>
#include "ui.h"
#include "color.h"

//Define properties shared amongst all UI componenets
#define FIELDS_COMMON_PROPERTIES(struct_type) \
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER, struct_type, name, 0, CYAML_UNLIMITED),\
    CYAML_FIELD_INT("id", CYAML_FLAG_DEFAULT, struct_type, id),\
    CYAML_FIELD_BOOL("visible", CYAML_FLAG_DEFAULT, struct_type, visible)\

#define FIELDS_COMMON_RECT(struct_type)\
    FIELDS_COMMON_PROPERTIES(struct_type),\
    CYAML_FIELD_INT("x", CYAML_FLAG_DEFAULT, struct_type, x),\
    CYAML_FIELD_INT("y", CYAML_FLAG_DEFAULT, struct_type, y),\
    CYAML_FIELD_INT("width", CYAML_FLAG_DEFAULT, struct_type, width),\
    CYAML_FIELD_INT("height", CYAML_FLAG_DEFAULT, struct_type, height)\

//Translate yaml colors to bzzt colors
static const cyaml_strval_t bzzt_color_strs[] = {
    { "black",         BZ_BLACK         },
    { "blue",          BZ_BLUE          },
    { "green",         BZ_GREEN         },
    { "cyan",          BZ_CYAN          },
    { "red",           BZ_RED           },
    { "magenta",       BZ_MAGENTA       },
    { "brown",         BZ_BROWN         },
    { "light-gray",    BZ_LIGHT_GRAY    },
    { "dark-gray",     BZ_DARK_GRAY     },
    { "light-blue",    BZ_LIGHT_BLUE    },
    { "light-green",   BZ_LIGHT_GREEN   },
    { "light-cyan",    BZ_LIGHT_CYAN    },
    { "light-red",     BZ_LIGHT_RED     },
    { "light-magenta", BZ_LIGHT_MAGENTA },
    { "yellow",        BZ_YELLOW        },
    { "white",         BZ_WHITE         },
};
#define BZZT_COLOR_CT  (int)(sizeof(bzzt_color_strs)/sizeof(bzzt_color_strs[0]))

//UIElement - text fields and schema
static const cyaml_schema_value_t text_fields[] = {
    //tbd
};

static const cyaml_schema_field_t text_schema = {
    //tbd
};
//UIElement - button fields and schema
static const cyaml_schema_field_t btn_fields[] = {
    FIELDS_COMMON_RECT(UIElement),
    CYAML_FIELD_STRING_PTR("text", CYAML_FLAG_POINTER, UIButton, ud, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t btn_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, UIButton, btn_fields)
};