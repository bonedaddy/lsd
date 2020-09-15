#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lora.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

byte hello[32] = "HELLO";

// SX1272 - Raspberry connections
int ssPin = 6;
int dio0  = 7;
int RST   = 0;

// Set spreading factor (SF7 - SF12)
sf_t sf = SF7;

// Set center frequency
uint32_t  freq = 868100000; // in Mhz! (868.1)

int main (int argc, char *argv[]) {

    if (argc < 2) {
        printf ("Usage: argv[0] sender|rec [message]\n");
        exit(1);
    }

    lora_client_opts_t opts = {
        .ss_pin = ssPin,
        .dio_0 = dio0,
        .rst = RST,
        .spi_channel = CHANNEL,
        .spi_speed = 500000,
        .config_power = 23,
        .frequency = freq,
        .sf = sf
    };

    lora_client_t *client = new_lora_client_t(opts);
    if (client == NULL) {
        printf("failed to start lora client\n");
        exit(1);
    }

    LOG_INFO(client->thl, 0, "lora client initialized");

    if (!strcmp("sender", argv[1])) {
        configure_sender(client);
        if (argc > 2)
            strncpy((char *)hello, argv[2], sizeof(hello));

        while(1) {
            txlora(client, hello, strlen((char *)hello));
            delay(5000);
        }
    } else {

        configure_receiver(client);

        while(1) {
            receive_packet(client); 
            delay(1);
        }

    }

    return (0);
}