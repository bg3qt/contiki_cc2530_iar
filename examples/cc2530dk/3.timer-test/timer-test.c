/**
 * \file
 *         Tests related to clocks and timers
 *
 *         This is clock_test.c plus a small addition by George Oikonomou
 *         (Loughborough University) in order to test the rtimer
 *
 * \author
 *         Zach Shelby <zach@sensinode.com> (Original)
 *         George Oikonomou - <oikonomou@users.sourceforge.net> (rtimer code)
 *
 */

#include "contiki.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "dev/leds.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
#define TEST_CLOCK_DELAY     1
#define TEST_RTIMER          1
#define TEST_ETIMER          1
#define TEST_CLOCK_SECONDS   1
/*---------------------------------------------------------------------------*/
static struct etimer et;

#if TEST_CLOCK_DELAY
static clock_time_t start_count, end_count, diff;
#endif

#if TEST_CLOCK_SECONDS
static unsigned long sec;
#endif

#if TEST_ETIMER
static clock_time_t count;
#endif

#if TEST_RTIMER
static struct rtimer rt;
rtimer_clock_t rt_now, rt_for;
static clock_time_t ct;
#endif

static uint8_t i;
/*---------------------------------------------------------------------------*/
PROCESS(clock_test_process, "clock test process");
AUTOSTART_PROCESSES(&clock_test_process);
/*---------------------------------------------------------------------------*/
#if TEST_RTIMER
void
rt_callback(struct rtimer *t, void *ptr) {
  rt_now = RTIMER_NOW();
  ct = clock_time();
  printf("Task called at %u (clock = %u)\r\n", rt_now, ct);
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(clock_test_process, ev, data)
{

  PROCESS_BEGIN();

  etimer_set(&et, 2 * CLOCK_SECOND);

  PROCESS_YIELD();

#if TEST_CLOCK_DELAY
  printf("\r\nClock delay test, (10,000 x i) cycles:\r\n");
  i = 1;
  while(i < 6) {
    start_count = clock_time();                   // 记录开始timer
    clock_delay(10000 * i);                       // 软件延时
    end_count = clock_time();                     // 记录结束timer
    diff = end_count - start_count;               // 计算差值，单位为tick
    printf("Delayed %u \n%u ticks =~ %u ms\r\n", 10000 * i, diff, diff * 8);
    i++;
  }
#endif

#if TEST_RTIMER
  printf("\r\nRtimer Test, 1 sec (%u rtimer ticks):\r\n", RTIMER_SECOND);
  i = 0;
  while(i < 5) {
    etimer_set(&et, 2*CLOCK_SECOND);              // etimer溢出时间2s
    ct = clock_time();                            // 记录当前etimer计数器
    rt_now = RTIMER_NOW();                        // 记录RTIMER起点时间   
    rt_for = rt_now + RTIMER_SECOND;              // 计算RTIMER终点时间
                                                  // 打印以上三个参数
    printf("Now = %u (clock = %u)\nFor = %u\n", rt_now, ct, rt_for);
    // 注册回调函数，回调函数应在RTIMER终点时间调用
    if (rtimer_set(&rt, rt_for, 1,
              (void (*)(struct rtimer *, void *))rt_callback, NULL) != RTIMER_OK) {
      printf("Error setting\n");
    }
    // 等待etimer溢出
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    i++;
  }
#endif

#if TEST_ETIMER
  printf("\r\nClock tick and etimer test, 1 sec (%u clock ticks):\r\n", CLOCK_SECOND);
  i = 0;
  while(i < 10) {
    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);

    count = clock_time();
    printf("%u ticks\r\n", count);

    leds_toggle(LEDS_RED);
    i++;
  }
#endif

#if TEST_CLOCK_SECONDS
  printf("\r\nClock Seconds Test (5s):\r\n");             // 间隔为5S
  i = 0;
  while(i < 10) {
    etimer_set(&et, 5 * CLOCK_SECOND);              // etimer溢出时间为5s
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));  // 等待定时器溢出
//    etimer_reset(&et);                             

    sec = clock_seconds();                          // 系统运行时间 单位s
    printf("%lu Seconds\r\n", sec);                   // 打印

//    leds_toggle(LEDS_GREEN);
    i++;
  }
#endif

  printf("Done!\r\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
