/* -*- mode: c; c-file-style: "bsd"; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/* taken from https://code.google.com/p/swmodem/ */

////////////////////////////////////////////////////////////////////////////////////////////////////
// crc16.c - 16-bit cyclic code calculator
//
// 2009 (C) Javier Valcarce García, <javier.valcarce@gmail.com>
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "crc16.h"

#define POLYNOMIAL 0x8005


////////////////////////////////////////////////////////////////////////////////////////////////////
// Update CRC
static uint16_t crc16_update(uint16_t crc, uint8_t a)
{
  // polynomial 0xA001 is the same as 0x8005
  // in reverse implementation
  uint8_t i;

  crc ^= (a << 8);
  for (i = 0; i < 8; i++)
    {
      if (crc & 0x8000)
	crc = (crc << 1) ^ POLYNOMIAL;
      else
	crc = (crc << 1);
    }

  return crc;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Appends a CRC at the end of the frame, at pkt[len - 1] and pkt[len - 2]
void crc16_append(uint8_t* pkt, uint8_t len)
{
  uint16_t crc = 0xFFFF;
  uint8_t  lsb;
  uint8_t  msb;
  uint8_t  i;

  for (i = 0; i < len - 2; i++)
    crc = crc16_update(crc, pkt[i]);

  // Is very important to maintain the portability to check the crc in this manner
  // and not using a pointer to uint16_t because due to the machine endianness this
  // would be a machine dependent code
  lsb = (crc & 0x00FF);
  msb = (crc & 0xFF00) >> 8;

  // CONVENTION: LITTLE-ENDIAN
  pkt[len - 2] = lsb;
  pkt[len - 1] = msb;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Check for valid frame, computes from pkt[0] to pkt[len-3] inclusive
uint8_t crc16_check(uint8_t* pkt, uint8_t len)
{
  uint16_t crc = 0xFFFF;
  uint8_t lsb;
  uint8_t  msb;
  uint8_t  i;

  for (i = 0; i < len - 2; i++)
    crc = crc16_update(crc, pkt[i]);

  // Is very important to maintain the portability to check the crc in this manner
  // and not using a pointer to uint16_t because due to the machine endianness this
  // would be a machine dependent code
  lsb = (crc & 0x00FF);
  msb = (crc & 0xFF00) >> 8;

  // CONVENTION: LITTLE-ENDIAN
  if (lsb != pkt[len - 2]) return 0;
  if (msb != pkt[len - 1]) return 0;

  return 1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
