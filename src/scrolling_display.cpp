#include <cstring>
#include "scrolling_display.h"

ScrollingDisplay::ScrollingDisplay(unsigned int clk, unsigned int dio, unsigned long interval)
    : display(clk, dio), scroll_position(0), buffer_index(0), buffer_swap_pending(false), interval(interval), scrolling(false) {
    clear();
}

void ScrollingDisplay::init() {
    display.clear();
    display.setBrightness(7);
}

void ScrollingDisplay::setup_scrolling(bool immediate) {
    char * text = current_buffer();
    if (strlen(text) <= 4) {
        // the text is short, disable scrolling
        scrolling = false;
        scroll_position = 0;
        ticker.detach();
        display.showText(text);
    } else {
        // scrolling needed
        scrolling = true;
        scroll_position = immediate ? -1 : -5;
        tick();
        ticker.attach(float(interval) * 0.001, [this](){ tick(); });
    }
}

void ScrollingDisplay::tick() {
    char * text = current_buffer();
    ++scroll_position;
    if (scroll_position <= -4) {
        display.clear();
    } else if (scroll_position < 0) {
        display.showText(text, 4 + scroll_position, -scroll_position);
    } else if (!text[scroll_position]) {
        display.clear();
        scroll_position = -4;
        if (buffer_swap_pending) {
            // need to switch buffers now
            buffer_index = 1 - buffer_index;
            buffer_swap_pending = false;
            setup_scrolling(false);
        }
    } else {
        display.showText(text + scroll_position);
    }
}

void ScrollingDisplay::set_text(const char * text, bool immediate) {
    strncpy(current_buffer(), text, SCROLLING_DISPLAY_BUFFER_SIZE);
    buffer_swap_pending = false;
    setup_scrolling(immediate);
}

void ScrollingDisplay::clear() {
    set_text("");
}

void ScrollingDisplay::set_next_text(const char * text) {
    if (scrolling) {
        // scrolling, show the next text when the current one scrolls out
        strncpy(other_buffer(), text, SCROLLING_DISPLAY_BUFFER_SIZE);
        buffer_swap_pending = true;
    } else {
        // current text is not scrolling, set next text immediately
        set_text(text, false);
    }
}
