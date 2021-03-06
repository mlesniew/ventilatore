#ifndef SCROLLING_DISPLAY_H
#define SCROLLING_DISPLAY_H

#include <Ticker.h>
#include <TM1637Display.h>

#define SCROLLING_DISPLAY_BUFFER_SIZE 200

class ScrollingDisplay {

public:
    ScrollingDisplay(unsigned int clk, unsigned int dio, unsigned long interval);

    void init();
    void tick();
    void set_text(const char * text, bool immediate = true);
    void set_next_text(const char * text);
    void set_brightness(unsigned char brightness, bool on = true);
    void clear();

protected:
    char * current_buffer() { return buffer[buffer_index]; }
    char * other_buffer() { return buffer[1 - buffer_index]; }
    void setup_scrolling(bool immediate);

    TM1637Display display;
    Ticker ticker;
    char buffer[2][SCROLLING_DISPLAY_BUFFER_SIZE];
    int scroll_position;
    unsigned int buffer_index;
    bool buffer_swap_pending;
    unsigned long interval;
    bool scrolling;
};
#endif
