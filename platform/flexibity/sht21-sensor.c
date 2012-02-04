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
#include "dev/sht21-sensor.h"
#include "mc1322x.h"
#include "i2c.h"

#undef DEBUG

enum {
  ON, OFF
};

const struct sensors_sensor sht21_sensor;
static rtimer_clock_t sht21_timer;
static uint8_t state = OFF;
uint8_t buf[16];

static int sht21_temp();
static int sht21_humidity();

static int
value(int type)
{
  int val = 0;

  switch (type) {
    case SHT21_SENSOR_TEMP:
      val = sht21_temp();
#ifdef DEBUG
      printf("sht21_temp: 0x%x\n", val);
#endif
      /* return temp * 100 (0.01 deg accuracy) */
      return (-46.85 + 175.72*((val>>16)&0xfffffffc)/0x10000)*100;
    case SHT21_SENSOR_HUMIDITY:
      val = sht21_humidity();
#ifdef DEBUG
      printf("sht21_humidity: 0x%x\n", val);
#endif
      /* return relative humidity * 100 (0.04 % accuracy) */
      return (-6 + 125*((val>>16)&0xfffffffc)/0x10000)*100;
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
          /* For for about 15ms before the SHT11 can be used */
          sht21_timer = RTIMER_NOW();
          while(RTIMER_CLOCK_LT(RTIMER_NOW(), sht21_timer + (RTIMER_SECOND/1000)*15));
        }
      } else {
        i2c_disable();
        state = OFF;
      }
  }

  return 0;
}


static int
sht21_temp()
{
  buf[0] = 0xe3;
  i2c_transmitinit(0x40, 1, buf);
  while(!i2c_transferred()) ;

  /* Wait for measurement about 85ms */
  sht21_timer = RTIMER_NOW();
  while(RTIMER_CLOCK_LT(RTIMER_NOW(), sht21_timer + (RTIMER_SECOND/1000)*85));

  i2c_receiveinit(0x40, 3, buf);
  while(!i2c_transferred()) ;

  return (int)(buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3]);
}


static int
sht21_humidity()
{
  buf[0] = 0xe5;
  i2c_transmitinit(0x40, 1, buf);
  while(!i2c_transferred()) ;

  /* Wait for measurement about 85ms */
  sht21_timer = RTIMER_NOW();
  while(RTIMER_CLOCK_LT(RTIMER_NOW(), sht21_timer + (RTIMER_SECOND/1000)*85));

  i2c_receiveinit(0x40, 3, buf);
  while(!i2c_transferred()) ;

  return (int)(buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3]);
}


SENSORS_SENSOR(sht21_sensor, "sht21",
	       value, configure, status);

