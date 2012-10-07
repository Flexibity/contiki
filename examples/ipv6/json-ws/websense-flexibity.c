/*
 * Copyright (c) 2012, Maxim Osipov
 * Copyright (c) 2011-2012, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         Websense for Flexibity mote
 * \author
 *         Maxim Osipov    <maxim.osipov@gmail.com>
 *         Niclas Finne    <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Joel Hoglund    <joel@sics.se>
 */

#include "contiki.h"
#include "dev/leds.h"
#include "battery.h"
#include "button.h"
#include "sht21.h"
#include "mpl115a2.h"
#include "jsontree.h"
#include "json-ws.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(websense_process, "Websense (Flexibity)");
AUTOSTART_PROCESSES(&websense_process);

static int
output_temp(struct jsontree_context *path)
{
  char buf[64];
  int tmp;

  tmp = sht21_temp();
  sprintf(buf, "%i.%i", tmp/100, tmp%100);
  jsontree_write_atom(path, buf);
  return 0;
}

static int
output_hum(struct jsontree_context *path)
{
  char buf[64];
  int tmp;

  tmp = sht21_humidity();
  sprintf(buf, "%i.%i", tmp/100, tmp%100);
  jsontree_write_atom(path, buf);
  return 0;
}

static int
output_pres(struct jsontree_context *path)
{
  char buf[64];
  int tmp;

  tmp = mpl115a2_pressure();
  sprintf(buf, "%i.%i", tmp/100, tmp%100);
  jsontree_write_atom(path, buf);
  return 0;
}

static struct jsontree_callback temp_callback =
  JSONTREE_CALLBACK(output_temp, NULL);

static struct jsontree_callback hum_callback =
  JSONTREE_CALLBACK(output_hum, NULL);

static struct jsontree_callback pres_callback =
  JSONTREE_CALLBACK(output_pres, NULL);

/*---------------------------------------------------------------------------*/

static struct jsontree_string desc = JSONTREE_STRING("Flexibity THAP");
static struct jsontree_string temp_unit = JSONTREE_STRING("degC");
static struct jsontree_string hum_unit = JSONTREE_STRING("%");
static struct jsontree_string pres_unit = JSONTREE_STRING("mbar");

JSONTREE_OBJECT(node_tree,
                JSONTREE_PAIR("node-type", &desc),
                JSONTREE_PAIR("time", &json_time_callback));

JSONTREE_OBJECT(temp_tree,
                JSONTREE_PAIR("unit", &temp_unit),
                JSONTREE_PAIR("value", &temp_callback));

JSONTREE_OBJECT(hum_tree,
                JSONTREE_PAIR("unit", &hum_unit),
                JSONTREE_PAIR("value", &hum_callback));

JSONTREE_OBJECT(pres_tree,
                JSONTREE_PAIR("unit", &pres_unit),
                JSONTREE_PAIR("value", &pres_callback));

JSONTREE_OBJECT(rsc_tree,
                JSONTREE_PAIR("temperature", &temp_tree),
                JSONTREE_PAIR("humidity", &hum_tree),
                JSONTREE_PAIR("pressure", &pres_tree));

/* complete node tree */
JSONTREE_OBJECT(tree,
                JSONTREE_PAIR("node", &node_tree),
                JSONTREE_PAIR("rsc", &rsc_tree),
                JSONTREE_PAIR("cfg", &json_subscribe_callback));

/*---------------------------------------------------------------------------*/
/* for cosm plugin */
#if WITH_COSM
/* set COSM value callback to be the temp sensor */
struct jsontree_callback cosm_value_callback =
  JSONTREE_CALLBACK(output_sensor, NULL);
#endif
PROCESS_THREAD(websense_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  json_ws_init(&tree);

  json_ws_set_callback("rsc");

  while(1) {
    /* Alive indication with the LED */
    etimer_set(&timer, CLOCK_SECOND * 5);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    leds_on(LEDS_RED);
    etimer_set(&timer, CLOCK_SECOND / 8);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    leds_off(LEDS_RED);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
