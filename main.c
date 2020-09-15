#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lora.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

byte hello[32] = "HELLO";

int main (int argc, char *argv[]) {

    if (argc < 2) {
        printf ("Usage: argv[0] sender|rec [message]\n");
        exit(1);
    }

    lora_client_opts_t opts = {.ss_pin = ssPin, .dio_0 = dio0, .rst = RST, .spi_channel = CHANNEL, .spi_speed = 500000};
    lora_client_t *client = new_lora_client_t(opts);
    if (client == NULL) {
        printf("failed to start lora client\n");
        exit(1);
    }

    LOG_INFO(client->thl, 0, "lora client initialized");

    if (!strcmp("sender", argv[1])) {
        // enter standby mode (required for FIFO loading))
        opmode(OPMODE_STANDBY);

        writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec

        configPower(23);

        printf("Send packets at SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
        printf("------------------\n");

        if (argc > 2)
            strncpy((char *)hello, argv[2], sizeof(hello));

        while(1) {
            txlora(hello, strlen((char *)hello));
            delay(5000);
        }
    } else {

        // radio init
        opmode(OPMODE_STANDBY);
        opmode(OPMODE_RX);

        LOGF_INFO(client->thl, 0, "listening at SF%i on %.6lf Mhz", sf,(double)freq/1000000);

        while(1) {
            receivepacket(); 
            delay(1);
        }

    }

    return (0);
}