/**
 * @file ui_action.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-10-04
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "debugger.h"
#include "engine.h"

// Initialize UI action registry
UIActionRegistry *UIAction_Registry_Create(void)
{
    UIActionRegistry *registry = malloc(sizeof(UIActionRegistry));
    if (!registry)
        return NULL;

    registry->count = 0;
    memset(registry->action_names, 0, sizeof(registry->action_names));
    memset(registry->handlers, 0, sizeof(registry->handlers));

    return registry;
}

// Destroy UI action registry
void UIAction_Registry_Destroy(UIActionRegistry *registry)
{
    if (!registry)
        return;

    for (int i = 0; i < registry->count; ++i)
    {
        if (registry->action_names[i])
        {
            free(registry->action_names[i]);
            registry->action_names[i] = NULL;
        }
    }

    free(registry);
}
// Register a C function with a string name
bool UIAction_Register(UIActionRegistry *registry, const char *name, void (*c_func)(UIActionContext *ctx))
{
    if (!registry || !name || !c_func)
        return false;

    if (registry->count >= UI_ACTION_REGISTRY_MAX)
    {
        Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Action registry full. Cannot register: %s", name);
        return false;
    }

    for (int i = 0; i < registry->count; ++i)
    {
        if (strcmp(registry->action_names[i], name) == 0)
        {
            Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Action %s already registered.", name);
            return false;
        }
    }

    registry->action_names[registry->count] = strdup(name);

    registry->handlers[registry->count].type = C_FUNCTION;
    registry->handlers[registry->count].handler.c_func = c_func;
    registry->count++;

    Debug_Log(LOG_LEVEL_DEBUG, LOG_UI, "Registered action '%s' (%d total)", name, registry->count);
    return true;
}
// Resolve action name to handler
bool UIAction_Resolve(UIActionRegistry *registry, const char *name, UIActionHandler *out_handler)
{
    if (!registry || !name || !out_handler)
        return false;

    for (int i = 0; i < registry->count; ++i)
    {
        if (strcmp(registry->action_names[i], name) == 0)
        {
            *out_handler = registry->handlers[i];
            return true;
        }
    }
    Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Action '%s' not found in registry.", name);
    return false;
}
// Execute action handler
void UIAction_Execute(UIActionHandler *handler, UIActionContext *ctx)
{
    if (!handler || !ctx)
        return;

    switch (handler->type)
    {
    case C_FUNCTION:
        if (handler->handler.c_func)
            handler->handler.c_func(ctx);
        break;
    case BOOP_FUNCTION:
        Debug_Log(LOG_LEVEL_WARN, LOG_UI, "BOOP script execution not yet implemented.");
        break;
    default:
        Debug_Log(LOG_LEVEL_ERROR, LOG_UI, "Unknown action handler type: %d", handler->type);
        break;
    }
}
// Resolve all button actions in loaded UI
void UI_Resolve_Button_Actions(UI *ui, UIActionRegistry *registry)
{
    if (!ui || !registry)
        return;

    for (int l = 0; l < ui->layer_count; ++l)
    {
        UILayer *layer = ui->layers[l];
        if (!layer)
            continue;

        for (int s = 0; s < layer->surface_count; ++s)
        {
            UISurface *surface = layer->surfaces[s];
            if (!surface)
                continue;

            for (int o = 0; o < surface->overlays_count; ++o)
            {
                UIOverlay *overlay = surface->overlays[o];
                if (!overlay)
                    continue;

                for (int e = 0; e < overlay->elements_count; ++e)
                {
                    UIElement *elem = overlay->elements[e];
                    if (!elem || elem->type != UI_ELEM_BUTTON)
                        continue;

                    UIButton *btn = (UIButton *)elem;

                    // Resolve on down action
                    if (btn->events.on_down_action)
                    {
                        if (UIAction_Resolve(registry, btn->events.on_down_action, &btn->events.on_down_handler))
                        {
                            Debug_Log(LOG_LEVEL_DEBUG, LOG_UI, "Button '%s' resolved on_down -> '%s'",
                                      btn->base.properties.name ? btn->base.properties.name : "Unnamed",
                                      btn->events.on_down_action);
                        }
                    }

                    // Resolve on release action
                    if (btn->events.on_release_action)
                    {
                        if (UIAction_Resolve(registry, btn->events.on_release_action, &btn->events.on_release_handler))
                        {
                            Debug_Log(LOG_LEVEL_DEBUG, LOG_UI, "Button '%s' resolved on_release -> '%s'",
                                      btn->base.properties.name ? btn->base.properties.name : "Unnamed",
                                      btn->events.on_release_action);
                        }
                    }

                    // Resolve on press action
                    if (btn->events.on_press_action)
                    {
                        if (UIAction_Resolve(registry, btn->events.on_press_action, &btn->events.on_press_handler))
                        {
                            Debug_Log(LOG_LEVEL_DEBUG, LOG_UI, "Button '%s' resolved on_press -> '%s'",
                                      btn->base.properties.name ? btn->base.properties.name : "Unnamed",
                                      btn->events.on_press_action);
                        }
                    }
                }
            }
        }
    }
}
// Update all butons to check inputs and fire events
void UI_Update_Button_Events(UI *ui, Engine *engine)
{
    if (!ui || !engine)
        return;

    for (int l = 0; l < ui->layer_count; ++l)
    {
        UILayer *layer = ui->layers[l];
        if (!layer || !layer->enabled)
            continue;

        for (int s = 0; s < layer->surface_count; ++s)
        {
            UISurface *surface = layer->surfaces[s];
            if (!surface || !surface->properties.enabled)
                continue;

            for (int o = 0; o < surface->overlays_count; ++o)
            {
                UIOverlay *overlay = surface->overlays[o];
                if (!overlay || !overlay->properties.enabled)
                    continue;

                for (int e = 0; e < overlay->elements_count; ++e)
                {
                    UIElement *elem = overlay->elements[e];
                    if (!elem || !elem->properties.enabled || elem->type != UI_ELEM_BUTTON)
                        continue;

                    UIButton *btn = (UIButton *)elem;

                    bool is_active = false;
                    for (int i = 0; i < btn->input_bindings.count; ++i)
                    {
                        UIInputBinding *binding = &btn->input_bindings.bindings[i];
                        switch (binding->type)
                        {
                        case KEYBOARD:
                            if (IsKeyDown(binding->code))
                                is_active = true;
                            break;
                        case MOUSE:
                            // tbd: check mouse pos over button
                            if (IsMouseButtonDown(binding->code))
                                is_active = true;
                            break;
                        case GAMEPAD:
                            if (IsGamepadButtonDown(0, binding->code))
                                is_active = true;
                            break;
                        }

                        if (is_active)
                            break;
                    }

                    static bool was_pressed[4096] = {0};
                    int btn_id = btn->base.properties.id;

                    // likely a problem
                    if (btn_id < 0 || btn_id >= 4096)
                        continue;

                    bool was_pressed_last = was_pressed[btn_id];
                    bool is_pressed_now = is_active;

                    UIActionContext ctx = {
                        .engine = engine,
                        .element = elem,
                        .button = btn,
                        .user_data = NULL};

                    // Fire on_down event
                    if (is_pressed_now && !was_pressed_last)
                    {
                        ctx.event_type = UI_EVENT_DOWN;
                        // if (btn->events.on_down_handler.type != 0)
                        if (btn->events.on_down_handler.handler.c_func)
                            UIAction_Execute(&btn->events.on_down_handler, &ctx);
                    }

                    // Fire on_press event
                    if (is_pressed_now && was_pressed_last)
                    {
                        ctx.event_type = UI_EVENT_PRESS;
                        // if (btn->events.on_press_handler.type != 0)
                        if (btn->events.on_press_handler.handler.c_func)
                            UIAction_Execute(&btn->events.on_press_handler, &ctx);
                    }

                    // Fire on_release event
                    if (!is_pressed_now && was_pressed_last)
                    {
                        ctx.event_type = UI_EVENT_RELEASE;
                        // if (btn->events.on_release_handler.type != 0)
                        if (btn->events.on_release_handler.handler.c_func)
                            UIAction_Execute(&btn->events.on_release_handler, &ctx);
                    }

                    was_pressed[btn_id] = is_pressed_now;
                }
            }
        }
    }
}