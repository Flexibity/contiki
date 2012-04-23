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
 *
 * This file is part of the Contiki operating system.
 *
 */

#include <stdlib.h>

#include "contiki.h"
#include "lib/sensors.h"
#include "dev/mpl115a2-sensor.h"
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


enum {
  ON, OFF
};

const struct sensors_sensor mpl115a2_sensor;
static rtimer_clock_t mpl115a2_timer;
static uint8_t state = OFF;
static uint8_t buf[16];

static int mpl115a2_temp();
static int mpl115a2_pressure();

static int
value(int type)
{
  int val = 0;

  switch (type) {
    case MPL115A2_SENSOR_TEMP:
      val = mpl115a2_temp();
#ifdef DEBUG
      printf("mpl115a2_temp: 0x%x\n", val);
#endif
      /* return temperature */
      return val;
    case MPL115A2_SENSOR_PRESSURE:
      val = mpl115a2_pressure();
#ifdef DEBUG
      printf("mpl115a2_pressure: 0x%x\n", val);
#endif
      /* return barometric pressure */
      return val;
  }

  return 0;
}


static int
status(int type)
{
  switch (type) {
    case SENSORS_ACTIVE:
    case SENSORS_READY:
      return (state == ON);
  }

  return 0;
}


static int
configure(int type, int c)
{
  switch (type) {
    case SENSORS_ACTIVE:
      if (c) {
        if (!status(SENSORS_ACTIVE)) {
          i2c_enable();
          state = ON;
          /* For for about 15ms before the MPL115A2 can be used */
          mpl115a2_timer = RTIMER_NOW();
          while(RTIMER_CLOCK_LT(RTIMER_NOW(), mpl115a2_timer + (RTIMER_SECOND/1000)*15));
        }
      } else {
        i2c_disable();
        state = OFF;
      }
  }

  return 0;
}


static int
mpl115a2_temp()
{
  buf[0] = START_TEMP_CONV;
  i2c_transmitinit(0x60, 1, buf);
  while(!i2c_transferred()) ;

  /* Wait for measurement about 1ms */
  mpl115a2_timer = RTIMER_NOW();
  while(RTIMER_CLOCK_LT(RTIMER_NOW(), mpl115a2_timer + (RTIMER_SECOND/1000)));

  buf[0] = POUTH;
  i2c_receiveinit(0x60, 1, buf);
  i2c_transmitinit(0x60, 2, buf);
  while(!i2c_transferred()) ;

  return (int)(buf[0]<<8 | buf[1]);
}


static int
mpl115a2_pressure()
{
  buf[0] = START_PRESS_CONV;
  i2c_transmitinit(0x60, 1, buf);
  while(!i2c_transferred()) ;

  /* Wait for measurement about 1ms */
  mpl115a2_timer = RTIMER_NOW();
  while(RTIMER_CLOCK_LT(RTIMER_NOW(), mpl115a2_timer + (RTIMER_SECOND/1000)));

  buf[0] = TOUTH;
  i2c_transmitinit(0x60, 1, buf);
  i2c_receiveinit(0x60, 2, buf);
  while(!i2c_transferred()) ;

  return (int)(buf[0]<<8 | buf[1]);
}


SENSORS_SENSOR(mpl115a2_sensor, "mpl115a2",
	       value, configure, status);

