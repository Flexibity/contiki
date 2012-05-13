/*
 * Copyright (c) 2012, Maxim Osipov <maxim.osipov@gmail.com>
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include "contiki.h"
#include "mpl115a2.h"
#include "mc1322x.h"
#include "i2c.h"

#define DEBUG

#define POUTH			0x00
#define POUTL			0x01
#define TOUTH			0x02
#define TOUTL			0x03
#define COEF1			0x04
#define COEF2			0x05
#define COEF3			0x06
#define COEF4			0x07
#define COEF5			0x08
#define COEF6			0x09
#define COEF7			0x0a
#define COEF8			0x0b
#define COEF9			0x0c
#define COEF10			0x0d
#define COEF11			0x0e
#define COEF12			0x0f
#define START_TEMP_CONV		0x10
#define START_PRESS_CONV	0x11
#define START_BOTH_CONV		0x12

static rtimer_clock_t mpl115a2_timer;
static uint8_t buf[16];

static int16_t sia0, sib1, sib2, sic12, sic11, sic22;
static int32_t si_c11x1, si_a11, si_c12x2, si_a1, si_c22x2, si_a2, si_a1x1, si_y1, si_a2x2, l1, l2, l3;

int32_t mpl115a2_pressure(void)
{
  int32_t val;
  uint16_t p, t;

  i2c_enable();

  buf[0] = START_BOTH_CONV;
  buf[1] = 0x01;
  i2c_transmitinit(0x60, 2, buf);
  while(!i2c_transferred()) ;

  /* Wait for measurement about 6ms */
  mpl115a2_timer = RTIMER_NOW();
  while(RTIMER_CLOCK_LT(RTIMER_NOW(), mpl115a2_timer + (RTIMER_SECOND/1000)*6));

  /* Read data and coefficients it once */
  buf[0] = POUTH;
  i2c_transmitinit(0x60, 1, buf);
  while(!i2c_transferred()) ;
  i2c_receiveinit(0x60, 16, buf);
  while(!i2c_transferred()) ;

  i2c_disable();

  p = (buf[0]<<8 | buf[1])>>6;
  t = (buf[2]<<8 | buf[3])>>6;
  sia0 = (buf[4+0]<<8 | buf[4+1]);
  sib1 = (buf[4+2]<<8 | buf[4+3]);
  sib2 = (buf[4+4]<<8 | buf[4+5]);
  sic12 = (buf[4+6]<<8 | buf[4+7]);
  sic11 = (buf[4+8]<<8 | buf[4+9]);
  sic22 = (buf[4+10]<<8 | buf[4+11]);

  si_c11x1 = (int32_t)sic11 * (int32_t)p;
  si_a11 = (((int32_t)sib1)<<14 + si_c11x1)>>14;
  si_c12x2 = (int32_t)sic12 * (int32_t)t;
  si_a1 = (si_a11<<11 + si_c12x2)>>11;
  si_c22x2 = (int32_t)sic22 * (int32_t)t;
  si_a2 = (((int32_t)sib2)<<15 + si_c22x2>>1)>>16;
  si_a1x1 = si_a1 * (int32_t)p;
  si_y1 = (((int32_t)sia0)<<10 + si_a1x1)>>10;
  si_a2x2 = si_a2 * (int32_t)t;
  l1 = ((int32_t)si_y1)<<10;
  l2 = (int32_t)si_a2x2;
  l3 = l1 + l2;

  val = (int16_t)(l3>>13);
  val = (65*100*10*val)/1023 + 50;

  return val;
}

