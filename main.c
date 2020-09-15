#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lora.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <argtable3.h>

byte hello[32] = "HELLO";

// SX1272 - Raspberry connections
int ssPin = 6;
int dio0  = 7;
int RST   = 0;

// Set spreading factor (SF7 - SF12)
sf_t sf = SF7;

// Set center frequency
uint32_t  freq = 868100000; // in Mhz! (868.1)

struct arg_lit *help;
struct arg_int *ss_pin, *dio_0, *rst;
struct arg_end *end;
struct arg_str *mode;

const char *client_mode = "receive";

int main (int argc, char *argv[]) {

    void *argtable[] = {
        mode = arg_strn(NULL, "mode", "<mode>", 0, 1, "mode to start client in"),
        help = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
        ss_pin = arg_intn(NULL, "sspin", "<n>", 0, 1, "sspin number"),
        dio_0 = arg_intn(NULL, "dio0", "<n>", 0, 1, "dio0 number"),
        rst = arg_intn(NULL, "rst", "<n>", 0, 1, "rst number"),
        end     = arg_end(20),
    };

    int nerrors = arg_parse(argc, argv, argtable);

    if (help->count > 0) {
        printf("Usage: %s", argv[0]);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Demonstrate command-line parsing in argtable3.\n\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        return 0;
    }

    if (nerrors > 0) {
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        arg_print_errors(stdout, end, argv[0]);
        return 1;
    }

    if (mode->count == 0) {
        mode->sval = &client_mode;
    }

    if (ss_pin->count == 0) {
        ss_pin->ival = &ssPin;
    }

    if (dio_0->count == 0) {
        dio_0->ival = &dio0;
    }

    if (rst->count == 0) {
        rst->ival = &RST;
    }

    printf("sspin: %i\tdio0: %i\trst: %i\n", *ss_pin->ival, *dio_0->ival, *rst->ival);
    printf("mode: %s\n", *mode->sval);

    lora_client_opts_t opts = {
        .ss_pin = *ss_pin->ival,
        .dio_0 = *dio_0->ival,
        .rst = *rst->ival,
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

    if (!strcmp("sender", *mode->sval)) {
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
   /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return (0);
}