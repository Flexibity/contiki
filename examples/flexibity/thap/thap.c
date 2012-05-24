/*
 * Copyright (c) 2012, Maxim Osipov <maxim.osipov@gmail.com>
 * Copyright (c) 2007, Swedish Institute of Computer Science
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest.h"
#include "dev/leds.h"
#include "mc1322x.h"

#include "battery.h"
#include "button.h"
#include "sht21.h"
#include "mpl115a2.h"
#include "readhex.h"


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

char buf[180];

RESOURCE(data, METHOD_GET, "index.html");
void
data_handler(REQUEST* request, RESPONSE* response)
{
  int temp, hum, pres;
  temp = sht21_temp();
  hum = sht21_humidity();
  pres = mpl115a2_pressure();
  sprintf(buf, "{ \"type\": \"thap\", \"version\": 1, \"temp\": %i.%i, \"hum\": %i.%i, \"pres\": %i.%i }\n",
	temp/100, temp%100, hum/100, hum%100, pres/100, pres%100);

  rest_set_header_content_type(response, TEXT_PLAIN);
  rest_set_response_payload(response, (uint8_t*)buf, strlen(buf));
}

PROCESS(flexibity_thap, "THAP Server");
AUTOSTART_PROCESSES(&flexibity_thap);
PROCESS_THREAD(flexibity_thap, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("HTTP Server\n");

  rest_init();
  rest_activate_resource(&resource_data);

  PRINTF("Started\n");
  PROCESS_END();
}

