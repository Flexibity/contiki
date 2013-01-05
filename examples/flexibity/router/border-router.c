/*
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
/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 *         Maxim Osipov <maxim.osipov@gmail.com>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
//#include "dev/button-sensor.h"
#include "dev/slip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEBUG DEBUG_NONE
#include "net/uip-debug.h"

uint16_t dag_id[] = {0x1111, 0x1100, 0, 0, 0, 0, 0, 0x0011};

extern uip_ds6_nbr_t uip_ds6_nbr_cache[];
extern uip_ds6_route_t uip_ds6_routing_table[];

static uip_ipaddr_t prefix;
static uint8_t prefix_set;

PROCESS(border_router_process, "Border router process");

/* Use simple webserver with only one page for minimum footprint.
 * Multiple connections can result in interleaved tcp segments since
 * a single static buffer is used for all segments.
 */
#include "httpd-simple.h"
/* The internal webserver can provide additional information if
 * enough program flash is available.
 */
#define WEBSERVER_CONF_LOADTIME 0
#define WEBSERVER_CONF_NEIGHBOR_STATUS 1

PROCESS(webserver_nogui_process, "Web server");
PROCESS_THREAD(webserver_nogui_process, ev, data)
{
  PROCESS_BEGIN();

  httpd_init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    httpd_appcall(data);
  }
  
  PROCESS_END();
}
AUTOSTART_PROCESSES(&border_router_process,&webserver_nogui_process);

static const char *TOP = "{ \"type\": \"router\", \"version\": 1,\n";
static const char *BOTTOM = "\n}\n";
static char buf[256];
static int blen;
#define ADD(...) do {                                                   \
    blen += snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);      \
  } while(0)

/*---------------------------------------------------------------------------*/
static void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) ADD("::");
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        ADD(":");
      }
      ADD("%x", a);
    }
  }
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(generate_routes(struct httpd_state *s))
{
  static int i;
  static uip_ds6_route_t *r;
#if WEBSERVER_CONF_LOADTIME
  static clock_time_t numticks;
  numticks = clock_time();
#endif

  PSOCK_BEGIN(&s->sout);

  SEND_STRING(&s->sout, TOP);
  blen = 0;
  ADD("\"neighbours\": [");
  for(i = 0; i < UIP_DS6_NBR_NB; i++) {
    if(uip_ds6_nbr_cache[i].isused) {
#if WEBSERVER_CONF_NEIGHBOR_STATUS
      ADD("\n { \"addr\": \"");
      ipaddr_add(&uip_ds6_nbr_cache[i].ipaddr);
      ADD("\", \"state\": \"");
      switch (uip_ds6_nbr_cache[i].state) {
      case NBR_INCOMPLETE: ADD("INCOMPLETE");break;
      case NBR_REACHABLE: ADD("REACHABLE");break;
      case NBR_STALE: ADD("STALE");break;      
      case NBR_DELAY: ADD("DELAY");break;
      case NBR_PROBE: ADD("NBR_PROBE");break;
      }
      ADD("\"},");
#else
      ipaddr_add(&uip_ds6_nbr_cache[i].ipaddr);
#endif
      if(blen > sizeof(buf) - 45) {
        SEND_STRING(&s->sout, buf);
        blen = 0;
      }
    }
  }
  ADD("\n { \"addr\": \"::1\", \"state\": \"REACHABLE\" }"); /* Dummy entry */
  ADD("\n],\n\"routes\": [");
  SEND_STRING(&s->sout, buf);
  blen = 0;
  for(r = uip_ds6_route_list_head(); r != NULL; r = list_item_next(r)) {
    ADD("\n { \"addr\": \"");
    ipaddr_add(&r->ipaddr);
    ADD("/%u\", \"hop\": \"", r->length);
    ipaddr_add(&r->nexthop);
    ADD("\",");
    if(1 || (r->state.lifetime < 600)) {
      ADD(" \"life\": %lu", r->state.lifetime);
    }
    SEND_STRING(&s->sout, buf);
    blen = 0;
    ADD(" },");
  }
  ADD("\n { \"addr\": \"::1\", \"hop\": \"::1\", \"life\": 1 }"); /* Dummy entry */
  ADD("\n]");

  SEND_STRING(&s->sout, buf);
  SEND_STRING(&s->sout, BOTTOM);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_simple_script_t
httpd_simple_get_script(const char *name)
{

  return generate_routes;
}


/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTA("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
void
request_prefix(void)
{
  /* mess up uip_buf with a dirty request... */
  uip_buf[0] = '?';
  uip_buf[1] = 'P';
  uip_len = 2;
  slip_send();
  uip_len = 0;
}
/*---------------------------------------------------------------------------*/
void
set_prefix_64(uip_ipaddr_t *prefix_64)
{
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);
  prefix_set = 1;
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;
  rpl_dag_t *dag;

  PROCESS_BEGIN();

/* While waiting for the prefix to be sent through the SLIP connection, the future
 * border router can join an existing DAG as a parent or child, or acquire a default 
 * router that will later take precedence over the SLIP fallback interface.
 * Prevent that by turning the radio off until we are initialized as a DAG root.
 */
  prefix_set = 0;
  NETSTACK_MAC.off(0);

  PROCESS_PAUSE();

//  SENSORS_ACTIVATE(button_sensor);

  PRINTF("RPL-Border router started\n");
#if 0
   /* The border router runs with a 100% duty cycle in order to ensure high
     packet reception rates.
     Note if the MAC RDC is not turned off now, aggressive power management of the
     cpu will interfere with establishing the SLIP connection */
  NETSTACK_MAC.off(1);
#endif
 
  /* Request prefix until it has been received */
  while(!prefix_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_prefix();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)dag_id);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }

  /* Now turn the radio on, but disable radio duty cycling.
   * Since we are the DAG root, reception delays would constrain mesh throughbut.
   */
  NETSTACK_MAC.off(1);
  
#if DEBUG || 1
  print_local_addresses();
#endif

  while(1) {
    PROCESS_YIELD();
//    if (ev == sensors_event && data == &button_sensor) {
//      PRINTF("Initiating global repair\n");
//      rpl_repair_root(RPL_DEFAULT_INSTANCE);
//    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
