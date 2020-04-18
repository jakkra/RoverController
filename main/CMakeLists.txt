idf_component_register(
    SRCS "rover_telematics.c" "rover_controller.c" "controller_input.c" "rover_transport.c" "main.c" "web_server.c"
    INCLUDE_DIRS ""
    EMBED_TXTFILES "certs/cacert.pem"
                    "certs/prvtkey.pem"
)
