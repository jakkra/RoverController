idf_component_register(
    SRCS "leds.c" "transport_ws.c" "transport_lora.c" "rover_telematics.c" "transport_lora.c" "rover_controller.c" "controller_input.c" "main.c" "web_server.c"
    INCLUDE_DIRS ""
    EMBED_TXTFILES "certs/cacert.pem"
                    "certs/prvtkey.pem"
)
