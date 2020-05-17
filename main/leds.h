#pragma once

typedef enum Led {
    LED_RIGHT,
    LED_LEFT,
    LED_END
} Led;

void leds_init(void);
void leds_toggle(Led num);