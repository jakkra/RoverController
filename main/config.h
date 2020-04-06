#define WS_SERVER_PORT  80

#define AP_SSID                             "RoverController"
#define AP_PASS                             ""

#define ROVER_WS_URL                        "ws://192.168.4.5:81"

// TODO could use mdns instead of hardcoding
#define ROVER_CONTROLLER_STATIC_IP          "192.168.4.1"
#define ROVER_STATIC_IP                     "192.168.4.5"
#define ROVER_CAM_STATIC_IP                 "192.168.4.6"

#define ROVER_CONTROLLER_MIN_TX_VALUE       1000
#define ROVER_CONTROLLER_MAX_TX_VALUE       2000

/*
TODO make something like this?
uint16_t rover_channel_map[6][2] = {
    { 0, INPUT_LEFT_JOYSTICK },
    { 1, INPUT_RIGHT_JOYSTICK },
    { 2, INPUT_LEFT_JOYSTICK_BUTTON },
    { 3, INPUT_RIGHT_JOYSTICK_BUTTON },
    { 4, INPOUT_SWITCH_ROVER_MODE },
    { 5, INPOUT_SWITCH_ARM_MODE },
};
*/