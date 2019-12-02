/* TFA pool temperature sensor
 *
 * Copyright (C) 2015 Alexandre Coffignal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/*
10 24 bits frames

    CCCCIIII IIIITTTT TTTTTTTT DDBF

- C: checksum, sum of nibbles - 1
- I: device id (changing only after reset)
- T: temperature
- D: channel number
- B: battery status
- F: first transmission
*/

#include "decoder.h"

static int tfa_pool_thermometer_callback(r_device *decoder, bitbuffer_t *bitbuffer) {
    bitrow_t *bb = bitbuffer->bb;
    data_t *data;
    int i,checksum,checksum_rx,device,channel,battery;
    int temp_raw;
    float temp_f;

    for (i = 1; i < 8; i++) {
        if (bitbuffer->bits_per_row[i] != 28) {
            /*10 24 bits frame*/
            return 0;
        }
    }

    checksum_rx = ((bb[1][0]&0xF0)>>4);
    device      = (((bb[1][0]&0xF)<<4)+((bb[1][1]&0xF0)>>4));
    temp_raw    = ((bb[1][1]&0xF)<<8)+bb[1][2];
    temp_f      = (temp_raw > 2048 ? temp_raw - 4096 : temp_raw) / 10.0;
    channel     = (signed short)((bb[1][3]&0xC0)>>6);
    battery     = ((bb[1][3]&0x20)>>5);

    checksum    = 0x0F & ( ( bb[1][0] & 0x0F ) +
                           ( bb[1][1] >> 4 )   +
                           ( bb[1][1] & 0x0F ) +
                           ( bb[1][2] >> 4 )   +
                           ( bb[1][2] & 0x0F ) +
                           ( bb[1][3] >> 4 ) - 1 );

    if (checksum_rx != checksum) {
    //    fprintf(stderr, "checksum_rx != checksum: %d %d\n", checksum_rx, checksum);
        return DECODE_FAIL_MIC;
    }

    data = data_make(
            "model",            "",                 DATA_STRING,    _X("TFA-Pool","TFA pool temperature sensor"),
            "id",               "Id",               DATA_INT,   device,
            "channel",          "Channel",          DATA_INT,   channel,
            "battery_ok",       "Battery",          DATA_STRING,    battery ? "OK" : "LOW",
            "temperature_C",    "Temperature",      DATA_FORMAT,    "%.01f C",  DATA_DOUBLE,    temp_f,
            "mic",              "Integrity",        DATA_STRING,    "CHECKSUM",
            NULL);
    decoder_output_data(decoder, data);

    return 1;

}

static char *output_fields[] = {
    "model",
    "id",
    "channel",
    "battery_ok",
    "temperature_C",
    "mic",
    NULL
};

r_device tfa_pool_thermometer = {
    .name          = "TFA pool temperature sensor",
    .modulation    = OOK_PULSE_PPM,
    .short_width   = 2000,
    .long_width    = 4600,
    .gap_limit     = 7800,
    .reset_limit   = 10000,
    .decode_fn     = &tfa_pool_thermometer_callback,
    .disabled      = 0,
    .fields        = output_fields,
};
