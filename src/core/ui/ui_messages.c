#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ui.h"
#include "ui_messages.h"
#include "color.h"
#include "bzzt.h"

#define ZZT_MESSAGE_MAX_CHARS 58

const char *const zzt_message_table[] = {
    "Bomb activated!",
    "Energizer - you are invincible",
    "You already have a %s key!",
    "You now have a %s key.",
    "Ammunition - 5 shots per container.",
    "Gems give you Health!",
    "The %s door is now open.",
    "The %s door is locked!",
    "Torch - used for lighting in the underground.",
    "Don't need a torch - room is not dark!",
    "You don't have any torches!",
    "You are blocked by an invisible wall.",
    "A path is cleared through the forest.",
    "A fake wall - secret passage!",
    "Your way is blocked by water.",
    "Ouch!",
    "Can't shoot in this place!",
    "You don't have any ammo!",
    "Game over - Press ESCAPE",
    "Room is dark - you need to light a torch!",
    "Running out of time!",
    "",
    "ERR: %s",
    "%s"
};

static int message_duration_ticks(Bzzt_World *w)
{
    return Bzzt_World_Message_Ticks(w);
}

UISurface *create_flashing_text_surface(UI *ui)
{
    UILayer *layer = UI_Add_New_Layer(ui, true, true);
    if (!layer)
        return NULL;

    // Pass NULL for name or use strdup for dynamically allocated names
    // NULL is fine if you don't need to find it by name later
    UISurface *surface = UILayer_Add_New_Surface(layer, NULL,
                                                 1001, true, true,
                                                 0, 24, 999, // x, y,
                                                 60, 1);     // w, h
    if (!surface)
        return NULL;

    UIOverlay *overlay = UISurface_Add_New_Overlay(surface, NULL,
                                                   1002,
                                                   0, 0, 0,  // x, y, z
                                                   60, 1, 0, // w, h,
                                                   true, true,
                                                   LAYOUT_NONE,
                                                   ANCHOR_BOTTOM_LEFT, ALIGN_CENTER, 0);
    if (!overlay)
        return NULL;

    // Use NULL instead of "" to avoid any pointer confusion
    UIElement_Text *text = UIText_Create(0, 0, COLOR_WHITE, COLOR_BLACK,
                                         false, ALIGN_CENTER,
                                         pass_through_caption, NULL,
                                         false);
    if (!text)
        return NULL;

    UIOverlay_Add_New_Element(overlay, (UIElement *)text);

    return surface;
}

static UIElement_Text *get_message_text_element(UI *ui)
{
    if (!ui || !ui->flashing_text_surface)
        return NULL;

    UISurface *surface = ui->flashing_text_surface;
    if (surface->overlays_count == 0)
        return NULL;

    UIOverlay *overlay = surface->overlays[0];
    if (!overlay || overlay->elements_count == 0)
        return NULL;

    UIElement *elem = overlay->elements[0];
    if (!elem || elem->type != UI_ELEM_TEXT)
        return NULL;

    return (UIElement_Text *)elem;
}

static void set_message_text_and_color(UI *ui, const char *text, Color_Bzzt fg)
{
    UIElement_Text *text_elem = get_message_text_element(ui);
    if (!text_elem)
        return;

    text_elem->ud = (void *)text;
    text_elem->fg = fg;
    text_elem->bg = COLOR_BLACK;
}

void UI_Flash_Message(UI *ui, Bzzt_World *w, zzt_message_t zzt_msg, ...)
{
    if (!ui)
        return;

    // Initialize the surface if needed
    if (!ui->flashing_text_surface)
        ui->flashing_text_surface = create_flashing_text_surface(ui);
    if (!ui->flashing_text_surface)
        return;

    // Check if this message should be shown
    if (!zzt_message_should_show(ui->shown_messages, zzt_msg))
        return;

    // Mark message as shown if it's a show-once message
    if (zzt_message_is_show_once(zzt_msg))
        zzt_message_mark_as_shown(&ui->shown_messages, zzt_msg);

    // Get the message format string
    const char *format = zzt_message_table[zzt_msg];
    if (!format || format[0] == '\0')
        return;

    // Format the message with variable arguments if needed
    char formatted_message[256];
    va_list args;
    va_start(args, zzt_msg);
    vsnprintf(formatted_message, sizeof(formatted_message), format, args);
    va_end(args);

    size_t source_len = strnlen(formatted_message, ZZT_MESSAGE_MAX_CHARS);
    char *message_with_spaces = malloc(source_len + 3);
    if (!message_with_spaces)
        return;
    message_with_spaces[0] = ' ';
    memcpy(message_with_spaces + 1, formatted_message, source_len);
    message_with_spaces[source_len + 1] = ' ';
    message_with_spaces[source_len + 2] = '\0';

    // Free old message if exists
    if (ui->message_text)
        free(ui->message_text);

    ui->message_text = message_with_spaces;

    ui->message_ticks_remaining = message_duration_ticks(w);
    ui->message_active = true;

    // Set initial color (9 + (P2 mod 7) where P2 starts at ticks_remaining)
    Color_Bzzt initial_color = BZZT_PALETTE[9 +
                                            (ui->message_ticks_remaining % 7)];
    set_message_text_and_color(ui, ui->message_text, initial_color);

    // Make sure the surface is visible
    UISurface_Set_Visible(ui->flashing_text_surface, true);
    UISurface_Set_Enabled(ui->flashing_text_surface, true);
}

void UI_Flash_Message_String(UI *ui, Bzzt_World *w, const char *message)
{
    if (!ui || !message)
        return;

    // Initialize the surface if needed
    if (!ui->flashing_text_surface)
        ui->flashing_text_surface = create_flashing_text_surface(ui);
    if (!ui->flashing_text_surface)
        return;

    size_t source_len = strnlen(message, ZZT_MESSAGE_MAX_CHARS);
    char *message_with_spaces = malloc(source_len + 3);
    if (!message_with_spaces)
        return;
    message_with_spaces[0] = ' ';
    memcpy(message_with_spaces + 1, message, source_len);
    message_with_spaces[source_len + 1] = ' ';
    message_with_spaces[source_len + 2] = '\0';

    // Free old message if exists
    if (ui->message_text)
        free(ui->message_text);

    ui->message_text = message_with_spaces;

    ui->message_ticks_remaining = message_duration_ticks(w);
    ui->message_active = true;

    // Set initial color
    Color_Bzzt initial_color = BZZT_PALETTE[9 +
                                            (ui->message_ticks_remaining % 7)];
    set_message_text_and_color(ui, ui->message_text, initial_color);

    // Make sure the surface is visible
    UISurface_Set_Visible(ui->flashing_text_surface, true);
    UISurface_Set_Enabled(ui->flashing_text_surface, true);
}

void UI_Update_Message_Timer(UI *ui)
{
    if (!ui || !ui->message_active)
        return;

    // Decrement tick counter
    ui->message_ticks_remaining--;

    // Check if message should disappear
    if (ui->message_ticks_remaining <= 0)
    {
        UI_Clear_Message(ui);
        return;
    }

    // Update color based on remaining ticks (ZZT formula: 9 + (P2 mod 7))
    Color_Bzzt new_color = BZZT_PALETTE[9 + (ui->message_ticks_remaining %
                                             7)];
    set_message_text_and_color(ui, ui->message_text, new_color);
}

void UI_Clear_Message(UI *ui)
{
    if (!ui)
        return;

    ui->message_active = false;
    ui->message_ticks_remaining = 0;

    // Clear the text element's ud pointer first to avoid dangling pointer
    UIElement_Text *text_elem = get_message_text_element(ui);
    if (text_elem)
        text_elem->ud = NULL;

    // Now free the message text
    if (ui->message_text)
    {
        free(ui->message_text);
        ui->message_text = NULL;
    }

    // Hide the surface
    if (ui->flashing_text_surface)
    {
        UISurface_Set_Visible(ui->flashing_text_surface, false);
        UISurface_Set_Enabled(ui->flashing_text_surface, false);
    }
}
