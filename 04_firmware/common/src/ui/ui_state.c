#include "ui/ui_state.h"

#include <stddef.h>
#include <string.h>

void ui_state_init(ui_state_t *state)
{
    if (state == NULL)
    {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->page = UI_PAGE_STATUS;
    state->mode = UI_MODE_BROWSE;
}

bool ui_state_update(ui_state_t *state, ui_event_flags_t events)
{
    if (state == NULL)
    {
        return false;
    }

    const ui_state_t previous = *state;
    bool mode_changed = false;

    if ((events & UI_EVENT_LINK_LOST) != 0u)
    {
        state->mode = UI_MODE_BROWSE;
        state->subpage = 0u;
        return memcmp(&previous, state, sizeof(*state)) != 0;
    }

    if ((events & UI_EVENT_INTERACT_OFF) != 0u)
    {
        state->mode = UI_MODE_BROWSE;
        state->subpage = 0u;
        mode_changed = true;
    }
    else if ((events & UI_EVENT_INTERACT_ON) != 0u)
    {
        state->mode = UI_MODE_INTERACT;
        state->subpage = 0u;
        mode_changed = true;
    }

    if (((events & UI_EVENT_ENTER) != 0u) &&
        (state->mode == UI_MODE_INTERACT))
    {
        state->enter_count++;
    }

    if (mode_changed)
    {
        return memcmp(&previous, state, sizeof(*state)) != 0;
    }

    if (state->mode == UI_MODE_BROWSE)
    {
        if ((events & UI_EVENT_LEFT) != 0u)
        {
            state->page = (state->page == UI_PAGE_STATUS)
                ? (ui_page_t)(UI_PAGE_COUNT - 1)
                : (ui_page_t)(state->page - 1);
            state->subpage = 0u;
        }
        else if ((events & UI_EVENT_RIGHT) != 0u)
        {
            state->page = (ui_page_t)((state->page + 1) % UI_PAGE_COUNT);
            state->subpage = 0u;
        }
    }
    else
    {
        const uint8_t subpage_count =
            ui_state_subpage_count(state->page);

        if ((events & UI_EVENT_LEFT) != 0u)
        {
            state->subpage = (state->subpage == 0u)
                ? (uint8_t)(subpage_count - 1u)
                : (uint8_t)(state->subpage - 1u);
        }
        else if ((events & UI_EVENT_RIGHT) != 0u)
        {
            state->subpage =
                (uint8_t)((state->subpage + 1u) % subpage_count);
        }
    }

    return memcmp(&previous, state, sizeof(*state)) != 0;
}

uint8_t ui_state_subpage_count(ui_page_t page)
{
    switch (page)
    {
        case UI_PAGE_RC:
            return 4u;

        case UI_PAGE_IMU:
            return 2u;

        case UI_PAGE_STATUS:
        default:
            return 1u;
    }
}
