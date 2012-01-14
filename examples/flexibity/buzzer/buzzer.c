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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "mc1322x.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

char temp[100];
int buzzer_state;

RESOURCE(discover, METHOD_GET, ".well-known/core");
void
discover_handler(REQUEST* request, RESPONSE* response)
{
  int index = 0;
  index += sprintf(temp + index, "%s,", "</id>;n=\"ID\"");
  index += sprintf(temp + index, "%s,", "</led>;n=\"Led\"");
  index += sprintf(temp + index, "%s,", "</button>;n=\"Button\"");
  index += sprintf(temp + index, "%s,", "</buzzer>;n=\"Buzzer\"");

  rest_set_response_payload(response, (uint8_t*)temp, strlen(temp));
  rest_set_header_content_type(response, APPLICATION_LINK_FORMAT);
}


RESOURCE(id, METHOD_GET, "id");
void
id_handler(REQUEST* request, RESPONSE* response)
{
  sprintf(temp,"Flexibity Buzzer version 0.1");

  rest_set_header_content_type(response, TEXT_PLAIN);
  rest_set_response_payload(response, (uint8_t*)temp, strlen(temp));
}


RESOURCE(button, METHOD_GET, "button");
void
button_handler(REQUEST* request, RESPONSE* response)
{
  int val = button_sensor.value(0);
  if (val) {
    sprintf(temp,"On");
  } else {
    sprintf(temp,"Off");
  }

  rest_set_header_content_type(response, TEXT_PLAIN);
  rest_set_response_payload(response, (uint8_t*)temp, strlen(temp));
}


RESOURCE(led, METHOD_POST | METHOD_PUT , "led");
void
led_handler(REQUEST* request, RESPONSE* response)
{
  char color[10];
  char mode[10];
  uint8_t led = 0;
  int success = 1;
  int ret;

  ret = rest_get_query_variable(request, "color", color, 10);
  if (ret) {
    PRINTF("color %s\n", color);

    if (!strcmp(color,"red")) {
      led = LEDS_RED;
    } else if(!strcmp(color,"green")) {
      led = LEDS_GREEN;
    } else {
      success = 0;
    }
  } else {
    success = 0;
  }

  ret = rest_get_post_variable(request, "mode", mode, 10);
  if (success && ret) {
    PRINTF("mode %s\n", mode);

    if (!strcmp(mode, "on")) {
      leds_on(led);
    } else if (!strcmp(mode, "off")) {
      leds_off(led);
    } else {
      success = 0;
    }
  } else {
    success = 0;
  }

  if (!success) {
    rest_set_response_status(response, BAD_REQUEST_400);
  }
}


RESOURCE(buzzer, METHOD_GET | METHOD_PUT | METHOD_POST, "buzzer");
void
buzzer_handler(REQUEST* request, RESPONSE* response)
{
  if (buzzer_state) {
    pwm_duty(TMR3, 0);
    buzzer_state = 0;
  } else {
    pwm_duty(TMR3, 10000);
    buzzer_state = 1;
  }
}


PROCESS(flexibity_buzzer, "Buzzer Server");
AUTOSTART_PROCESSES(&flexibity_buzzer);
PROCESS_THREAD(flexibity_buzzer, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);
  pwm_init_stopped(TMR3, 1500, 10000);
  buzzer_state = 0;

#ifdef WITH_COAP
  PRINTF("COAP Server\n");
#else
  PRINTF("HTTP Server\n");
#endif

  rest_init();

  rest_activate_resource(&resource_buzzer);
  rest_activate_resource(&resource_led);
  rest_activate_resource(&resource_button);
  rest_activate_resource(&resource_id);
  rest_activate_resource(&resource_discover);

  PROCESS_END();
}

