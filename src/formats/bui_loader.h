/**
 * @file bui_loader.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-04
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <stdbool.h>

struct UI;

// Load a .bui file and append its contents to the given UI
bool UI_Load_From_BUI(struct UI *ui, const char *path);