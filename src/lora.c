/*******************************************************************************
 *
 * Copyright (c) 2018 Dragino
 *
 * http://www.dragino.com
 *
 * Copyright (c) 2020 Bonedaddy (Alex Trottier)
 * http://bonedaddy.io
 *
 *******************************************************************************/

#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "lora.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

byte receivedbytes;

/*!
 * @brief when set to true, causes the event loop to exit
 */
bool exit_event_loop = false;

/*!
 * @brief "internal" function that sets exit_event_loop
 */
void handle_signal();

/*!
 * @brief "internal" function that sets exit_event_loop
 */
void handle_signal() {
    exit_event_loop = true;
}

/*!
 * @brief used to free up resources allocated for lora_client_t
 */
void free_lora_client_t(lora_client_t *client) {
    clear_thread_logger(client->thl);
    free(client);
}

/*!
 * @brief main event loop
 */
void event_loop_lora_client_t(lora_client_t *client, bool mode_receive, byte *data) {
    if (mode_receive == false) {

        configure_sender(client);

        while (1) {
            if (exit_event_loop == true) {
                return;
            }
            txlora(client, data, strlen((char *)data));
            delay(5000);
        }

    } else {

        configure_receiver(client);
        char buffer[256];

        while (1) {
            if (exit_event_loop == true) {
                return;
            }
            memset(buffer, 0, 256);
            receive_packet(client, buffer);
            LOGF_INFO(client->thl, 0, "received a message: %s", buffer);
            delay(1);
        }
    }
}

/*!
 * @brief returns a new lora client initializing the onboard device
 * @warning it is not safe to return multiple clients, as this will
 * @warning override the setting of the other
 */
lora_client_t *new_lora_client_t(lora_client_opts_t opts) {
    thread_logger *thl = new_thread_logger(true);
    if (thl == NULL) {
        return NULL;
    }

    lora_client_t *client = calloc(1, sizeof(lora_client_t));
    if (client == NULL) {
        clear_thread_logger(thl);
        return NULL;
    }

    client->thl = thl;
    client->opts = opts;

    wiringPiSetup();
    pinMode(opts.ss_pin, OUTPUT);
    pinMode(opts.dio_0, INPUT);
    pinMode(opts.rst, OUTPUT);

    wiringPiSPISetup(opts.spi_channel, opts.spi_speed);

    setup_lora(client);

    // register signal handling functions
    // these will set a boolean indicating the
    // event loop should exit
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGQUIT, handle_signal);

    return client;
}

/*!
 * @brief configures the client for sending lora packets
 */
void configure_sender(lora_client_t *client) {
    // enter standby mode (required for FIFO loading))
    opmode(client, OPMODE_STANDBY);

    writeReg(client, RegPaRamp,
             (readReg(client, RegPaRamp) & 0xF0) |
                 0x08); // set PA ramp-up time 50 uSec

    configPower(client, client->opts.config_power);

    LOGF_INFO(client->thl, 0, "sending packets at SF%i on %.6lf Mhz",
              client->opts.sf, (double)client->opts.frequency / 1000000);
}

/*!
 * @brief configures the client for receiving lora packets
 */
void configure_receiver(lora_client_t *client) {
    // radio init
    opmode(client, OPMODE_STANDBY);
    opmode(client, OPMODE_RX);

    LOGF_INFO(client->thl, 0, "listening at SF%i on %.6lf Mhz", client->opts.sf,
              (double)client->opts.frequency / 1000000);
}

void die(const char *s) {
    perror(s);
    exit(1);
}

void select_receiver(lora_client_t *client) {
    digitalWrite(client->opts.ss_pin, LOW);
}

void unselect_receiver(lora_client_t *client) {
    digitalWrite(client->opts.ss_pin, HIGH);
}

byte readReg(lora_client_t *client, byte addr) {
    unsigned char spibuf[2];

    select_receiver(client);
    spibuf[0] = addr & 0x7F;
    spibuf[1] = 0x00;
    wiringPiSPIDataRW(CHANNEL, spibuf, 2);
    unselect_receiver(client);

    return spibuf[1];
}

void writeReg(lora_client_t *client, byte addr, byte value) {
    unsigned char spibuf[2];

    spibuf[0] = addr | 0x80;
    spibuf[1] = value;
    select_receiver(client);
    wiringPiSPIDataRW(CHANNEL, spibuf, 2);

    unselect_receiver(client);
}

void opmode(lora_client_t *client, uint8_t mode) {
    writeReg(client, REG_OPMODE,
             (readReg(client, REG_OPMODE) & ~OPMODE_MASK) | mode);
}

void opmodeLora(lora_client_t *client) {
    uint8_t u = OPMODE_LORA;
    if (sx1272 == false)
        u |= 0x8; // TBD: sx1276 high freq
    writeReg(client, REG_OPMODE, u);
}

void setup_lora(lora_client_t *client) {

    digitalWrite(client->opts.rst, HIGH);
    delay(100);
    digitalWrite(client->opts.rst, LOW);
    delay(100);

    byte version = readReg(client, REG_VERSION);

    if (version == 0x22) {
        // sx1272
        LOG_INFO(client->thl, 0, "SX1272 board detected, starting");
        sx1272 = true;
    } else {
        // sx1276?
        digitalWrite(client->opts.rst, LOW);
        delay(100);
        digitalWrite(client->opts.rst, HIGH);
        delay(100);
        version = readReg(client, REG_VERSION);
        if (version == 0x12) {
            // sx1276
            LOG_INFO(client->thl, 0, "SX1276 board detected, starting");
            sx1272 = false;
        } else {
            LOG_ERROR(client->thl, 0, "unrecognized transceiver");
            // printf("Version: 0x%x\n",version);
            exit(1);
        }
    }

    opmode(client, OPMODE_SLEEP);
    opmodeLora(client);

    // set frequency
    uint64_t frf = ((uint64_t)client->opts.frequency << 19) / 32000000;
    writeReg(client, REG_FRF_MSB, (uint8_t)(frf >> 16));
    writeReg(client, REG_FRF_MID, (uint8_t)(frf >> 8));
    writeReg(client, REG_FRF_LSB, (uint8_t)(frf >> 0));

    writeReg(client, REG_SYNC_WORD, 0x34); // LoRaWAN public sync word

    if (sx1272) {
        if (client->opts.sf == SF11 || client->opts.sf == SF12) {
            writeReg(client, REG_MODEM_CONFIG, 0x0B);
        } else {
            writeReg(client, REG_MODEM_CONFIG, 0x0A);
        }
        writeReg(client, REG_MODEM_CONFIG2, (client->opts.sf << 4) | 0x04);
    } else {
        if (client->opts.sf == SF11 || client->opts.sf == SF12) {
            writeReg(client, REG_MODEM_CONFIG3, 0x0C);
        } else {
            writeReg(client, REG_MODEM_CONFIG3, 0x04);
        }
        writeReg(client, REG_MODEM_CONFIG, 0x72);
        writeReg(client, REG_MODEM_CONFIG2, (client->opts.sf << 4) | 0x04);
    }

    if (client->opts.sf == SF10 || client->opts.sf == SF11 ||
        client->opts.sf == SF12) {
        writeReg(client, REG_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        writeReg(client, REG_SYMB_TIMEOUT_LSB, 0x08);
    }
    writeReg(client, REG_MAX_PAYLOAD_LENGTH, 0x80);
    writeReg(client, REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    writeReg(client, REG_HOP_PERIOD, 0xFF);
    writeReg(client, REG_FIFO_ADDR_PTR, readReg(client, REG_FIFO_RX_BASE_AD));

    writeReg(client, REG_LNA, LNA_MAX_GAIN);
}

bool receive(lora_client_t *client, char *payload) {
    // clear rxDone
    writeReg(client, REG_IRQ_FLAGS, 0x40);

    int irqflags = readReg(client, REG_IRQ_FLAGS);

    //  payload crc: 0x20
    if ((irqflags & 0x20) == 0x20) {
        printf("CRC error\n");
        writeReg(client, REG_IRQ_FLAGS, 0x20);
        return false;
    } else {

        byte currentAddr = readReg(client, REG_FIFO_RX_CURRENT_ADDR);
        byte receivedCount = readReg(client, REG_RX_NB_BYTES);
        receivedbytes = receivedCount;

        writeReg(client, REG_FIFO_ADDR_PTR, currentAddr);

        for (int i = 0; i < receivedCount; i++) {
            payload[i] = (char)readReg(client, REG_FIFO);
        }
    }
    return true;
}

void receive_packet(lora_client_t *client, char *buffer) {

    long int SNR;
    int rssicorr;

    if (digitalRead(client->opts.dio_0) == 1) {
        if (receive(client, buffer)) {
            byte value = readReg(client, REG_PKT_SNR_VALUE);
            if (value & 0x80) // The SNR sign bit is 1
            {
                // Invert and divide by 4
                value = ((~value + 1) & 0xFF) >> 2;
                SNR = -value;
            } else {
                // Divide by 4
                SNR = (value & 0xFF) >> 2;
            }

            if (sx1272) {
                rssicorr = 139;
            } else {
                rssicorr = 157;
            }
            LOGF_INFO(client->thl, 0,
                      "packet rssi: %d, rssi: %d, snr: %li, length: %i, payload: %s",
                      readReg(client, 0x1A) - rssicorr,
                      readReg(client, 0x1B) - rssicorr, SNR, (int)receivedbytes,
                      buffer);

        } // received a message

    } // dio0=1
}

void configPower(lora_client_t *client, int8_t pw) {
    if (sx1272 == false) {
        // no boost used for now
        if (pw >= 17) {
            pw = 15;
        } else if (pw < 2) {
            pw = 2;
        }
        // check board type for BOOST pin
        writeReg(client, RegPaConfig, (uint8_t)(0x80 | (pw & 0xf)));
        writeReg(client, RegPaDac, readReg(client, RegPaDac) | 0x4);

    } else {
        // set PA config (2-17 dBm using PA_BOOST)
        if (pw > 17) {
            pw = 17;
        } else if (pw < 2) {
            pw = 2;
        }
        writeReg(client, RegPaConfig, (uint8_t)(0x80 | (pw - 2)));
    }
}

void writeBuf(lora_client_t *client, byte addr, byte *value, byte len) {
    unsigned char spibuf[256];
    spibuf[0] = addr | 0x80;
    for (int i = 0; i < len; i++) {
        spibuf[i + 1] = value[i];
    }
    select_receiver(client);
    wiringPiSPIDataRW(CHANNEL, spibuf, len + 1);
    unselect_receiver(client);
}

void txlora(lora_client_t *client, byte *frame, byte datalen) {

    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(client, RegDioMapping1,
             MAP_DIO0_LORA_TXDONE | MAP_DIO1_LORA_NOP | MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(client, REG_IRQ_FLAGS, 0xFF);
    // mask all IRQs but TxDone
    writeReg(client, REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

    // initialize the payload size and address pointers
    writeReg(client, REG_FIFO_TX_BASE_AD, 0x00);
    writeReg(client, REG_FIFO_ADDR_PTR, 0x00);
    writeReg(client, REG_PAYLOAD_LENGTH, datalen);

    // download buffer to the radio FIFO
    writeBuf(client, REG_FIFO, frame, datalen);
    // now we actually start the transmission
    opmode(client, OPMODE_TX);

    printf("send: %s\n", frame);
}
