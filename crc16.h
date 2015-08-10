/* -*- mode: c; c-file-style: "bsd"; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/* taken from https://code.google.com/p/swmodem/ */

////////////////////////////////////////////////////////////////////////////////////////////////////
// crc16.h - 16-bit cyclic code calculator
//
// 2009 (C) Javier Valcarce García, <javier.valcarce@gmail.com>
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CRC16_H
#define CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//uint16_t crc16_update (uint16_t crc, uint8_t a  );
uint8_t  crc16_check  (uint8_t* pkt, uint8_t len);
void     crc16_append (uint8_t* pkt, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif
