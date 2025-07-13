/**
 * @file ui_bindings.h
 * @author your name (you@domain.com)
 * @brief Tools for binding data to UI elements for dynamic behavior
 * @version 0.1
 * @date 2025-07-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <stdio.h>
#include "color.h"

typedef enum
{
    BIND_INT,
    BIND_FLOAT,
    BIND_STR
} BindType;

/**
 * @brief Data container for UI text elements that need to print changing data per frame
 *
 */
typedef struct
{
    const void *ptr; // address of a variable
    const char *fmt; // printf style format string
    BindType type;
    char buf[32]; // Scratch buffer
} TextBinding;

/**
 * @brief Initialize a TextBinding
 *
 * @param pts
 * @param fmt
 * @param type
 * @return TextBinding*
 */
TextBinding *UIBinding_Text_Create(const void *ptr, const char *fmt, BindType type);

/**
 * @brief Callback function that formats a variable into a C string and copies it into a buffer
 *
 * @param ud
 * @return const char*
 */
const char *UIBinding_Text_Format(void *ud);