/* This is a very simple hello_world program.
 * It aims to demonstrate the co-existence of two processes:
 * One of them prints a hello world message and the other blinks the LEDs
 *
 * It is largely based on hello_world in $(CONTIKI)/examples/sensinode
 *
 * Author: George Oikonomou - <oikonomou@users.sourceforge.net>
 * Aletrado para a disciplina de Rede de Sensores Sem Fio
 */

#include "contiki.h"
#include "dev/leds.h"

#include <stdio.h> /* For printf() */

#define LED_PING_EVENT (44)
#define LED_PONG_EVENT (45)
/*---------------------------------------------------------------------------*/
static struct etimer et_hello;
static struct etimer et_blink;
static struct etimer et_proc3;
static struct etimer et_pong;
static uint16_t count;
//static uint8_t blinks;
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
PROCESS(blink_process, "LED blink process");
PROCESS(proc3_process, "Proc3 process");
PROCESS(pong_process, "Pong process");
AUTOSTART_PROCESSES(&blink_process,&hello_world_process,&proc3_process,&pong_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();

  etimer_set(&et_hello, CLOCK_SECOND * 4);
  count = 0;
  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER) {
      printf("Hello World enviando Ping\n");
      printf("Hello world #%u!\n", count);
      count++;
      etimer_reset(&et_hello);
      process_post (&pong_process, LED_PING_EVENT , (void *)(&hello_world_process));
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(proc3_process, ev, data)
{
  PROCESS_BEGIN();

  etimer_set(&et_proc3, CLOCK_SECOND * 5);

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER) {
      printf("Proc3 enviando Ping\n");
      leds_toggle(LEDS_RED);
      etimer_reset(&et_proc3);
      process_post (&pong_process, LED_PING_EVENT , (void *)(&proc3_process));
    }
    else if(ev == LED_PONG_EVENT){
      leds_toggle(LEDS_RED);
      printf("Proc3 recebeu Pong\n");
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(pong_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == LED_PING_EVENT) {
      if (data==&hello_world_process){
	  		  printf("Enviando pong para Hello World\n");
      }
      else if(data==&proc3_process){
		  	  printf("Enviando pong para Proc3\n");
      }
      else if(data==&blink_process){
    	   	  printf("Enviando pong para LED Blink\n");
      }
      process_post (( struct process *)data, LED_PONG_EVENT , NULL);
      leds_on(LEDS_BLUE);
      etimer_set(&et_pong, CLOCK_SECOND * 0.2);
    }
    else if(ev==PROCESS_EVENT_TIMER){
    	leds_off(LEDS_BLUE);
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(blink_process, ev, data)
{
  PROCESS_BEGIN();

  //blinks = 0;
  printf("LEDS_ALL = %d\n", LEDS_ALL);

  leds_off(LEDS_ALL);
  etimer_set(&et_blink, 5*CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
  etimer_set(&et_blink, 2*CLOCK_SECOND);
  while(1) {


    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER){
		printf("Blink enviando Ping\n");
		leds_toggle(LEDS_YELLOW);
		etimer_reset(&et_blink);
		//leds_on(blinks & LEDS_ALL);
		//blinks++;
		printf("Piscando o LED amarelo!\n", leds_get());
		process_post (&pong_process, LED_PING_EVENT , (void *)(&blink_process));
    }
    else if(ev == LED_PONG_EVENT){
        leds_toggle(LEDS_YELLOW);
        printf("Blink recebeu Pong\n");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
