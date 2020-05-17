#include "rover_telematics.h"
#include "assert.h"
#include "leds.h"

static on_telematics* on_data = NULL;


void rover_telematics_register_on_data(on_telematics* callback)
{
    assert(on_data == NULL);
    assert(callback != NULL);
    on_data = callback;
}

void rover_telematics_put(uint8_t* telematics, uint16_t length)
{
    // Later one LED will be used for something else.
    leds_toggle(LED_LEFT);
    leds_toggle(LED_RIGHT);
    if (on_data) {
        on_data(telematics, length);
    }
}