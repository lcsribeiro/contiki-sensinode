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

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/uip-debug.h"
#include "net/rpl/rpl.h"
#include "dev/sensinode-sensors.h"
#include "sensinode-debug.h"
#include "dev/watchdog.h"
#include "dev/slip.h"

static uip_ipaddr_t prefix;
static uint8_t prefix_set;

uint16_t dag_id[] = {0x1111, 0x1100, 0, 0, 0, 0, 0, 0x0011};
/*---------------------------------------------------------------------------*/
PROCESS(border_router_process, "Border Router process");
AUTOSTART_PROCESSES(&border_router_process);
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state
        == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* Tentative -> Preferred to finialise our address */
      if (state == ADDR_TENTATIVE) {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
request_prefix(void) {
  /* mess up uip_buf with a dirty request... */
  uip_buf[0] = '!';
  uip_buf[1] = 'P';
  uip_len = 2;
  slip_send();
}
/*---------------------------------------------------------------------------*/
static void
print_stats()
{
  PRINTF("tl=%lu, ts=%lu, bs=%lu, bc=%lu\n",
      rimestats.toolong, rimestats.tooshort, rimestats.badsynch, rimestats.badcrc);
  PRINTF("llrx=%lu, lltx=%lu, rx=%lu, tx=%lu\n",
      rimestats.llrx, rimestats.lltx, rimestats.rx, rimestats.tx);
}
/*---------------------------------------------------------------------------*/
/* Set our prefix when we receive one over SLIP */
void
set_prefix_64(uip_ipaddr_t *prefix_64) {
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
  static struct sensors_sensor *b1;
  static struct sensors_sensor *b2;
  static struct etimer et;
  rpl_dag_t *dag;

  PROCESS_BEGIN();
  PRINTF("Border Router started\n");
  prefix_set = 0;

  /* Request prefix until it has been received */
  while(!prefix_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_prefix();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  dag = rpl_set_root((uip_ip6addr_t *)dag_id);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }

  print_local_addresses();

  b1 = sensors_find(BUTTON_1_SENSOR);
  b2 = sensors_find(BUTTON_2_SENSOR);

  while(1) {
    PROCESS_YIELD();

    if(ev == sensors_event && data != NULL) {
        if(data == b1) {
          print_stats();
        } else if(data == b2) {
          watchdog_reboot();
        }
      }
    }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/