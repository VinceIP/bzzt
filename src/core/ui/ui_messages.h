#pragma once
#include <stdint.h>
#include <stdbool.h>

// A zzt system message, typically shown in ugly flashing text.
typedef enum zzt_message_t
{
    ZZT_MSG_BOMB_ACTIVATED,
    ZZT_MSG_ENERGIZER_ACTIVATED,
    ZZT_MSG_KEY_ALREADY_HAVE,
    ZZT_MSG_KEY_GET,
    ZZT_MSG_AMMO_GET,
    ZZT_MSG_GEM_GET,
    ZZT_MSG_DOOR_OPEN,
    ZZT_MSG_DOOR_LOCKED,
    ZZT_MSG_TORCH_GET,
    ZZT_MSG_TORCH_NOT_DARK,
    ZZT_MSG_TORCH_EMPTY,
    ZZT_MSG_TOUCH_INVISIBLE_WALL,
    ZZT_MSG_TOUCH_FOREST,
    ZZT_MSG_TOUCH_FAKE,
    ZZT_MSG_TOUCH_WATER,
    ZZT_MSG_OUCH,
    ZZT_MSG_SHOT_FORBIDDEN,
    ZZT_MSG_SHOT_EMPTY,
    ZZT_MSG_GAME_OVER,
    ZZT_MSG_ROOM_DARK,
    ZZT_MSG_ROOM_TIME_LOW,
    ZZT_MSG_EMPTY,
    ZZT_MSG_ERROR,
    ZZT_MSG_STRING_ONE_LINER
} zzt_message_t;

// Bitfield tracker for which one-time messages have been shown.
typedef uint32_t zzt_message_shown_flags_t;

// Define which messages are shown once only.
#define ZZT_MESSAGE_SHOW_ONCE_MASK (       \
    (1U << ZZT_MSG_AMMO_GET) |             \
    (1U << ZZT_MSG_GEM_GET) |              \
    (1U << ZZT_MSG_TORCH_GET) |            \
    (1U << ZZT_MSG_TORCH_NOT_DARK) |       \
    (1U << ZZT_MSG_TORCH_EMPTY) |          \
    (1U << ZZT_MSG_TOUCH_FOREST) |         \
    (1U << ZZT_MSG_TOUCH_FAKE) |           \
    (1U << ZZT_MSG_TOUCH_INVISIBLE_WALL) | \
    (1U << ZZT_MSG_ENERGIZER_ACTIVATED) |  \
    (1U << ZZT_MSG_SHOT_EMPTY) |           \
    (1U << ZZT_MSG_ROOM_DARK))

extern const char *const zzt_message_table[];

// Return true if a one-time message was already shown
static inline bool zzt_message_was_shown(zzt_message_shown_flags_t flags, zzt_message_t msg)
{
    return (flags & (1U << msg)) != 0;
}

static inline void zzt_message_mark_as_shown(zzt_message_shown_flags_t *flags, zzt_message_t msg)
{
    *flags |= (1U << msg);
}

// Return true if message should only be shown once.
static inline bool zzt_message_is_show_once(zzt_message_t msg)
{
    return (ZZT_MESSAGE_SHOW_ONCE_MASK & (1U << msg)) != 0;
}

static inline bool zzt_message_should_show(zzt_message_shown_flags_t flags, zzt_message_t msg)
{
    if (!zzt_message_is_show_once(msg))
    {
        return true; // Always show messages that can repeat
    }
    return !zzt_message_was_shown(flags, msg); // Show once messages only if not yet shown
}
