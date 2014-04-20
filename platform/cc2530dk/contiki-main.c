#include "contiki.h"
#include "soc.h"
#include "sys/clock.h"
#include "sys/autostart.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/leds.h"
#include "dev/uart0.h"
#include "dev/dma.h"
#include "dev/cc2530-rf.h"
#include "dev/watchdog.h"
#include "dev/clock-isr.h"
#include "dev/lpm.h"
#include "dev/button-sensor.h"
#include "dev/adc-sensor.h"
#include "dev/leds-arch.h"
#include "net/rime.h"
#include "net/netstack.h"
#include "net/mac/frame802154.h"
#include "debug.h"
#include "cc253x.h"
#include "sfr-bits.h"
#include "contiki-lib.h"
#include "contiki-net.h"
/*---------------------------------------------------------------------------*/
#if VIZTOOL_CONF_ON
PROCESS_NAME(viztool_process);
#endif
/*---------------------------------------------------------------------------*/
#ifdef STARTUP_CONF_VERBOSE
#define STARTUP_VERBOSE     STARTUP_CONF_VERBOSE
#else
#define STARTUP_VERBOSE     0
#endif

#if STARTUP_VERBOSE
#define PUTSTRING(...)      putstring(__VA_ARGS__)
#define PUTHEX(...)         puthex(__VA_ARGS__)
#define PUTBIN(...)         putbin(__VA_ARGS__)
#define PUTCHAR(...)        putchr(__VA_ARGS__)
#else
#define PUTSTRING(...)      do {} while(0)
#define PUTHEX(...)         do {} while(0)
#define PUTBIN(...)         do {} while(0)
#define PUTCHAR(...)        do {} while(0)
#endif
/*---------------------------------------------------------------------------*/
extern rimeaddr_t rimeaddr_node_addr;
static __data int r;
static __data int len;
/*---------------------------------------------------------------------------*/
#if ENERGEST_CONF_ON
  static unsigned long irq_energest = 0;
  #define ENERGEST_IRQ_SAVE(a) do { \
      a = energest_type_time(ENERGEST_TYPE_IRQ); } while(0)
  #define ENERGEST_IRQ_RESTORE(a) do { \
      energest_type_set(ENERGEST_TYPE_IRQ, a); } while(0)
#else
  #define ENERGEST_IRQ_SAVE(a) do {} while(0)
  #define ENERGEST_IRQ_RESTORE(a) do {} while(0)
#endif
/*---------------------------------------------------------------------------*/
static void
fade(int l)
{
  volatile int i, a;
  int k, j;
  for(k = 0; k < 400; ++k) {
    j = k > 200? 400 - k: k;

    leds_on(l);
    for(i = 0; i < j; ++i) {
      a = i;
    }
    leds_off(l);
    for(i = 0; i < 200 - j; ++i) {
      a = i;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_rime_addr(void)
{
  uint8_t *addr_long = NULL;
  uint16_t addr_short = 0;
  int8_t i;

#ifdef SDCC
  __xdata unsigned char * macp = &X_IEEE_ADDR;
#else
  volatile unsigned char * macp = &X_IEEE_ADDR;   // IEEE地址 位于XDATA 780C
#endif

  PUTSTRING("Rime is 0x");
  PUTHEX(sizeof(rimeaddr_t));
  PUTSTRING(" bytes long\r\n");

  PUTSTRING("Reading MAC from Info Page\r\n");    // 从INFO中取MAC地址，

  for(i = (RIMEADDR_SIZE - 1); i >= 0; --i) {
    rimeaddr_node_addr.u8[i] = *macp;
    macp++;
  }

  /* Now the address is stored MSB first */
#if STARTUP_VERBOSE                               // 显示MAC地址
  PUTSTRING("Rime configured with address ");
  for(i = 0; i < RIMEADDR_SIZE - 1; i++) {
    PUTHEX(rimeaddr_node_addr.u8[i]);
    PUTCHAR(':');
  }
  PUTHEX(rimeaddr_node_addr.u8[i]);
  PUTCHAR('\r');PUTCHAR('\n');
  PUTSTRING("**************************************\r\n");
#endif

  cc2530_rf_set_addr(IEEE802154_PANID);           // 设置PANID
}
/*---------------------------------------------------------------------------*/
int
main(void)
{
  clock_init();                                   // 初始化 睡眠定时器 必要
  soc_init();                                     // 还函数中启动了全局中断 可修改
  rtimer_init();                                  // rtimer为定时器1 必要

  /* Init LEDs here */
  leds_init();                                    // 初始化LED 可修改
  leds_off(LEDS_ALL);                             // 关闭所有LED 非必要
  fade(LEDS_GREEN);                               // 绿色闪烁一下 非必要

  /* initialize process manager. */
  process_init();                                 // 任务初始化 必要

  /* Init UART */
  uart0_init();                                   // 初始化串口0，先用于调试，可修改

#if DMA_ON
  dma_init();                                     // 非必要
#endif

#if SLIP_ARCH_CONF_ENABLE
  slip_arch_init(0);
#else
  uart0_set_input(serial_line_input_byte);
  serial_line_init();
#endif
  fade(LEDS_RED);                                 // 红色LED闪烁一下 非必要

  // 打印若干提示信息 非必要 可修改
  putstring("**************************************\r\n");
  putstring(CONTIKI_VERSION_STRING "\r\n");       // 打印若干信息
  putstring("Platform CC2530 NB\r\n");
  switch(CHIPID) {
  case 0xA5:
    putstring("CC2530");
    break;
  case 0xB5:
    putstring("CC2531");
    break;
  case 0x95:
    putstring("CC2533");
    break;
  case 0x8D:
    putstring("CC2540");
    break;
  }

  putstring("-F");
  switch(CHIPINFO0 & 0x70) {
  case 0x40:
    putstring("256,");
    break;
  case 0x30:
    putstring("128,");
    break;
  case 0x20:
    putstring("64,");
    break;
  case 0x10:
    putstring("32,");
    break;
  }
  puthex(CHIPINFO1 + 1);
  putstring("KB SRAM\r\n");

#if STARTUP_VERBOSE

  PUTSTRING("Net: ");                      // NETWORK名称
  PUTSTRING(NETSTACK_NETWORK.name);
  PUTCHAR('\r');PUTCHAR('\n');              
  PUTSTRING("MAC: ");                      // MAC名称
  PUTSTRING(NETSTACK_MAC.name);
  PUTCHAR('\r');PUTCHAR('\n');
  PUTSTRING("RDC: ");                      // RDC名称
  PUTSTRING(NETSTACK_RDC.name);
  PUTCHAR('\r');PUTCHAR('\n');
  PUTSTRING("**************************************\r\n");
#endif

  watchdog_init();                         // 初始化看门狗

  /* Initialise the H/W RNG engine. */
  random_init(0);                           //

  /* start services */
  process_start(&etimer_process, NULL);     // 启动etimer任务
  ctimer_init();                            // ctimer初始化 

  /* initialize the netstack */
  netstack_init();                          // NET协议栈初始化
  set_rime_addr();                          // 设置RIME地址，相当于设置IP地址

#if BUTTON_SENSOR_ON || ADC_SENSOR_ON
  process_start(&sensors_process, NULL);
  BUTTON_SENSOR_ACTIVATE();
  ADC_SENSOR_ACTIVATE();
#endif

#if UIP_CONF_IPV6                         // 非常重要，启动TCPIP查询任务
  memcpy(&uip_lladdr.addr, &rimeaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
  process_start(&tcpip_process, NULL);
#endif /* UIP_CONF_IPV6 */

#if VIZTOOL_CONF_ON
  process_start(&viztool_process, NULL);
#endif

  energest_init();                        // 能量估计初始化，但是该功能未被打开
  ENERGEST_ON(ENERGEST_TYPE_CPU);         // 该功能未被打开

  autostart_start(autostart_processes);   // 启动被定义为自动启动的任务

  watchdog_start();                       // 看门狗初始化     

  fade(LEDS_YELLOW);                      // 黄色LED闪烁，完成所有初始化工作

  while(1) {
    do {
      /* Reset watchdog and handle polls and events */
      watchdog_periodic();                // 喂狗操作
      r = process_run();
    } while(r > 0);
#if SHORTCUTS_CONF_NETSTACK               // 循环查询无线输入数据包长度 tcpip_process
    len = NETSTACK_RADIO.pending_packet();
    if(len) {
      packetbuf_clear();
      len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);
      if(len > 0) {
        packetbuf_set_datalen(len);
        NETSTACK_RDC.input();
      }
    }
#endif

#if LPM_MODE                              // 该宏被定义为0，没有休眠功能，以下代码均无效
#if (LPM_MODE==LPM_MODE_PM2)
    SLEEP &= ~OSC_PD;            /* Make sure both HS OSCs are on */
    while(!(SLEEP & HFRC_STB));  /* Wait for RCOSC to be stable */
    CLKCON |= OSC;               /* Switch to the RCOSC */
    while(!(CLKCON & OSC));      /* Wait till it's happened */
    SLEEP |= OSC_PD;             /* Turn the other one off */
#endif /* LPM_MODE==LPM_MODE_PM2 */

    /*
     * Set MCU IDLE or Drop to PM1. Any interrupt will take us out of LPM
     * Sleep Timer will wake us up in no more than 7.8ms (max idle interval)
     */
    SLEEPCMD = (SLEEPCMD & 0xFC) | (LPM_MODE - 1);

#if (LPM_MODE==LPM_MODE_PM2)
    /*
     * Wait 3 NOPs. Either an interrupt occurred and SLEEP.MODE was cleared or
     * no interrupt occurred and we can safely power down
     */
    __asm
      nop
      nop
      nop
    __endasm;

    if(SLEEPCMD & SLEEP_MODE0) {
#endif /* LPM_MODE==LPM_MODE_PM2 */

      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      ENERGEST_ON(ENERGEST_TYPE_LPM);

      /* We are only interested in IRQ energest while idle or in LPM */
      ENERGEST_IRQ_RESTORE(irq_energest);

      /* Go IDLE or Enter PM1 */
      PCON |= PCON_IDLE;

      /* First instruction upon exiting PM1 must be a NOP */
      __asm
        nop
      __endasm;

      /* Remember energest IRQ for next pass */
      ENERGEST_IRQ_SAVE(irq_energest);

      ENERGEST_ON(ENERGEST_TYPE_CPU);
      ENERGEST_OFF(ENERGEST_TYPE_LPM);

#if (LPM_MODE==LPM_MODE_PM2)
      SLEEPCMD &= ~SLEEP_OSC_PD;            /* Make sure both HS OSCs are on */
      while(!(SLEEPCMD & SLEEP_XOSC_STB));  /* Wait for XOSC to be stable */
      CLKCONCMD &= ~CLKCONCMD_OSC;              /* Switch to the XOSC */
      /*
       * On occasion the XOSC is reported stable when in reality it's not.
       * We need to wait for a safeguard of 64us or more before selecting it
       */
      clock_delay(10);
      while(CLKCONCMD & CLKCONCMD_OSC);         /* Wait till it's happened */
    }
#endif /* LPM_MODE==LPM_MODE_PM2 */
#endif /* LPM_MODE */
  }
}
/*---------------------------------------------------------------------------*/
