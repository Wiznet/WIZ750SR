#include <stdbool.h>

#include "W7500x_dualtimer.h"
#include "mbtimer.h"
#include "common.h"

volatile eMBRcvState eRcvState;
volatile uint8_t mb_state_rtu_finish;     

static volatile uint32_t mb_timeout_us = 0;  // T3.5 + ÀÀ´äÅ¸ÀÓ¾Æ¿ô (us)

static uint32_t us_to_timer_load(uint32_t usec)
{
    uint64_t ticks = ((uint64_t)GetSystemClock() * (uint64_t)usec) / 1000000ULL;
    if (ticks == 0) ticks = 1; // ÃÖ¼Ò 1Æ½
    if (ticks > 0xFFFFFFFFULL) ticks = 0xFFFFFFFFULL;
    return (uint32_t)ticks;
}

void xMBPortTimersInit(uint32_t usTim1Timerout50us)
{
    uint32_t t35_us;
		uint32_t sum = 0;
		DUALTIMER_InitTypDef cfg;
	
    if (usTim1Timerout50us > (0xFFFFFFFFUL / 50UL)) t35_us = 0xFFFFFFFFUL;
    else t35_us = usTim1Timerout50us * 50UL;

    sum = t35_us + 50000UL;
    if (sum < t35_us) sum = 0xFFFFFFFFUL; 

    mb_timeout_us = sum;

    DUALTIMER_ClockEnable(DUALTIMER0_1);

    cfg.TimerLoad              = us_to_timer_load(1000); 
    cfg.TimerControl_Mode      = DUALTIMER_TimerControl_OneShot;   
    cfg.TimerControl_OneShot   = DUALTIMER_TimerControl_OneShot;   
    cfg.TimerControl_Pre       = DUALTIMER_TimerControl_Pre_1;     
    cfg.TimerControl_Size      = DUALTIMER_TimerControl_Size_32;

    DUALTIMER_Init(DUALTIMER0_1, &cfg);
    DUALTIMER_IntConfig(DUALTIMER0_1, ENABLE);  
	
    printf("mb_timeout = %lu us\r\n", (unsigned long)mb_timeout_us);
}

void vMBPortTimersEnable(void)
{
    DUALTIMER_Stop(DUALTIMER0_1);
    DUALTIMER_SetTimerLoad(DUALTIMER0_1, us_to_timer_load(mb_timeout_us));
    DUALTIMER_Start(DUALTIMER0_1);
}

void vMBPortTimersDisable(void)
{
    DUALTIMER_Stop(DUALTIMER0_1);
    if (DUALTIMER_GetIntStatus(DUALTIMER0_1)) {
        DUALTIMER_IntClear(DUALTIMER0_1);
    }
}

void xMBRTUTimerT35Expired(void)
{
		//printf("xMBRTUTimerT35Expired expired!\n");
    switch (eRcvState) {
        case STATE_RX_INIT:
            break;

        case STATE_RX_RCV:
						//printf("xMBRTUTimerT35Expired : state RX RCV\n");
            mb_state_rtu_finish = true;
            break;

        case STATE_RX_ERROR:
            break;

        default:
            break;
    }

    vMBPortTimersDisable();
    eRcvState = STATE_RX_IDLE;
		//printf("Change State to STATE_RX_IDLE\n");
}

