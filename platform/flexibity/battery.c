/*
 * Copyright (c) 2012, Maxim Osipov <maxim.osipov@gmail.com>
 * Copyright (c) 2010, Mariano Alvira <mar@devl.org> and other contributors
 * to the MC1322x project (http://mc1322x.devl.org) and Contiki.
 *
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
 * This file is part of the Contiki OS.
 *
 */

#include "battery.h"
#include "mc1322x.h"
#include <signal.h>
#include <clock.h>
#include <timer.h>

/*
 * For now, a sensor is simple, polling only and without interrupts
 */
static struct timer adc_timer;

int battery(void)
{
  int val = 0;

  /* ADC for power and water (ADC1/GPIO_31)
   *   24MHz peripherial clock
   *   300kHz ADC clock
   *   1MHz prescale clock
   */
  ADC->CONTROLbits.ON = 0;
  ADC->CONTROLbits.FIFO_IRQ_MASK = 0;
  ADC->CONTROLbits.SEQ1_IRQ_MASK = 0;
  ADC->CONTROLbits.SEQ2_IRQ_MASK = 0;
  ADC->CONTROLbits.COMPARE_IRQ_MASK = 0;
  ADC->CONTROLbits.AD1_VREFHL_EN = 0;
  ADC->CONTROLbits.AD2_VREFHL_EN = 0;
  ADC->CONTROLbits.TIMER1_ON = 0;
  ADC->CONTROLbits.TIMER2_ON = 0;
  ADC->CLOCK_DIVIDER = 0x0050;
  ADC->PRESCALE = 0x17;
  ADC->ON_TIME = 0x000a;
  ADC->CONVERT_TIME = 0x0014;
  ADC->OVERRIDEbits.MUX1 = 0x8;
  ADC->OVERRIDEbits.AD1_ON = 1;
  ADC->MODE = 1;

  /* wait for ON (10us) + CONVERT (20us) time (x2 to be sure) */
  timer_set(&adc_timer, (CLOCK_SECOND/1000)*60);
  while (!timer_expired(&adc_timer));

  /* return % of 3.3V */
  val = (ADC->RESULT_1 > 0) ? (int)((0xfff*1.2*100)/(val*3.3)) : 0;

  ADC->CONTROLbits.ON = 0;
  ADC->MODE = 0;

  return val > 0 ? (int)((0xfff*1.2*100)/(val*3.3)) : 0;
}

