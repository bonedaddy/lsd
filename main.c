#include <argtable3.h>
#include <lora.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

byte hello[32] = "HELLO";

// override for compile time frequency selection
#ifndef USE_FREQUENCY
#define USE_FREQUENCY FREQ_868_MHZ
#endif

// SX1272 - Raspberry connections
int ssPin = 6;
int dio0 = 7;
int RST = 0;

// Set spreading factor (SF7 - SF12)
sf_t sf = SF7;

struct arg_lit *help;
struct arg_int *ss_pin, *dio_0, *rst;
struct arg_end *end;
struct arg_str *mode;

const char *client_mode = "receive";

int main(int argc, char *argv[]) {

    void *argtable[] = {
        mode = arg_strn(NULL, "mode", "<mode>", 0, 1, "mode to start client in"),
        help = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
        ss_pin = arg_intn(NULL, "sspin", "<n>", 0, 1, "sspin number"),
        dio_0 = arg_intn(NULL, "dio0", "<n>", 0, 1, "dio0 number"),
        rst = arg_intn(NULL, "rst", "<n>", 0, 1, "rst number"),
        end = arg_end(20),
    };

    // parse cli arguments
    int nerrors = arg_parse(argc, argv, argtable);

    // handle help flag early
    if (help->count > 0) {
        printf("Usage: %s", argv[0]);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Demonstrate command-line parsing in argtable3.\n\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        return 0;
    }

    // parse errors if any
    if (nerrors > 0) {
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        arg_print_errors(stdout, end, argv[0]);
        return 1;
    }

    // handle default values pulled in from globals

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

    bool mode_receive = true;
    if (strcmp("sender", *mode->sval) == 0) {
        mode_receive = false;
    }

    // prepare lora client options
    lora_client_opts_t opts = {.ss_pin = *ss_pin->ival,
                               .dio_0 = *dio_0->ival,
                               .rst = *rst->ival,
                               .spi_channel = CHANNEL,
                               .spi_speed = 500000,
                               .config_power = 23,
                               .frequency = USE_FREQUENCY,
                               .sf = sf};

    // initialize the lora client
    lora_client_t *client = new_lora_client_t(opts);
    if (client == NULL) {
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        printf("failed to start lora client\n");
        exit(1);
    }

    LOG_INFO(client->thl, 0, "lora client initialized");

    // start the main event loop
    event_loop_lora_client_t(client, mode_receive, hello);

    // clear up allocated resources for lora_client_t
    free_lora_client_t(client);

    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return (0);
}