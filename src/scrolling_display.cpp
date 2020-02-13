#include <cstring>
#include "scrolling_display.h"

ScrollingDisplay::ScrollingDisplay(unsigned int clk, unsigned int dio, unsigned long interval)
    : display(clk, dio), scroll_position(0), buffer_index(0), buffer_swap_pending(false) {
    set_text("");
    set_speed(interval);
}

void ScrollingDisplay::init() {
    display.clear();
    display.setBrightness(7);
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
            buffer_index = 1 - buffer_index;
            buffer_swap_pending = false;
        }
    } else {
        display.showText(text + scroll_position);
    }
}

void ScrollingDisplay::set_speed(unsigned long interval) {
    ticker.attach(float(interval) * 0.001, [this](){ tick(); });
}

void ScrollingDisplay::set_text(const char * text, bool immediate) {
    strncpy(current_buffer(), text, SCROLLING_DISPLAY_BUFFER_SIZE);
    if (immediate)
        scroll_position = 0;
    else
        scroll_position = -4;
    buffer_swap_pending = false;
}

void ScrollingDisplay::set_next_text(const char * text) {
    strncpy(other_buffer(), text, SCROLLING_DISPLAY_BUFFER_SIZE);
    buffer_swap_pending = true;
}
