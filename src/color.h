#pragma once
#include <stdint.h>
typedef struct
{
    uint8_t r, g, b;
} Color_bzzt;

static const Color_bzzt COLOR_BLACK        = {   0,   0,   0 };
static const Color_bzzt COLOR_BLUE         = {   0,   0, 170 };
static const Color_bzzt COLOR_GREEN        = {   0, 170,   0 };
static const Color_bzzt COLOR_CYAN         = {   0, 170, 170 };
static const Color_bzzt COLOR_RED          = { 170,   0,   0 };
static const Color_bzzt COLOR_MAGENTA      = { 170,   0, 170 };
static const Color_bzzt COLOR_BROWN        = { 170,  85,   0 };
static const Color_bzzt COLOR_LIGHT_GRAY   = { 170, 170, 170 };
static const Color_bzzt COLOR_DARK_GRAY    = {  85,  85,  85 };
static const Color_bzzt COLOR_LIGHT_BLUE   = {  85,  85, 255 };
static const Color_bzzt COLOR_LIGHT_GREEN  = {  85, 255,  85 };
static const Color_bzzt COLOR_LIGHT_CYAN   = {  85, 255, 255 };
static const Color_bzzt COLOR_LIGHT_RED    = { 255,  85,  85 };
static const Color_bzzt COLOR_LIGHT_MAGENTA= { 255,  85, 255 };
static const Color_bzzt COLOR_YELLOW       = { 255, 255,  85 };
static const Color_bzzt COLOR_WHITE        = { 255, 255, 255 };