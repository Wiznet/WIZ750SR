#include <stdint.h>
#include <stdbool.h>

#include "mbtimer.h"
#include "common.h"
#include "W7500x_board.h"
#include "W7500x_dualtimer.h"

volatile eMBRcvState eRcvState;

volatile uint8_t mb_state_rtu_finish;

void xMBPortTimersInit(uint32_t usTim1Timerout50us) {
    /* Calculate mb_timeout in ��s: T3.5 + 50ms response timeout */
    uint32_t mb_timeout;
    uint32_t t35_time_us = usTim1Timerout50us * 50;
    DUALTIMER_InitTypDef Dualtimer_InitStructure;
  
    if (usTim1Timerout50us > (0xFFFFFFFFUL / 50)) {
        mb_timeout = 0xFFFFFFFFUL;    // Prevent overflow
    } else {
        mb_timeout = t35_time_us + 50000;    // T3.5 + 50ms
    }

    /* Check for overflow */
    if (mb_timeout < t35_time_us) {
        mb_timeout = 0xFFFFFFFFUL;
    }
    printf("mb_timeout = %d us\r\n", mb_timeout);

    NVIC_EnableIRQ(DUALTIMER1_IRQn);

    /* Dualtimer 1_0 clock enable */
    DUALTIMER_ClockEnable(DUALTIMER1_0);

    /* Dualtimer 1_0 configuration */
    //Dualtimer_InitStructure.TimerLoad = (GetSystemClock() / 1000000) * mb_timeout;
    Dualtimer_InitStructure.TimerLoad = (GetSystemClock() / 1000000) * mb_timeout * 5; // * 5 margin
    printf("TimerLoad value = %u\r\n", Dualtimer_InitStructure.TimerLoad);
    Dualtimer_InitStructure.TimerControl_Mode = DUALTIMER_TimerControl_Periodic;
    Dualtimer_InitStructure.TimerControl_OneShot = DUALTIMER_TimerControl_Wrapping;
    Dualtimer_InitStructure.TimerControl_Pre = DUALTIMER_TimerControl_Pre_1;
    Dualtimer_InitStructure.TimerControl_Size = DUALTIMER_TimerControl_Size_32;

    DUALTIMER_Init(DUALTIMER1_0, &Dualtimer_InitStructure);

    /* Dualtimer 1_0 Interrupt enable */
    DUALTIMER_IntConfig(DUALTIMER1_0, ENABLE);

    /* Dualtimer 1_0 start */
    //DUALTIMER_Start(DUALTIMER1_0);
}

void vMBPortTimersEnable(void) {
    // Instead of starting a timer, we'll use manual delay-based timing
    // The actual delay will happen in xMBRTUTimerT35Expired when called manually
    
    // Start the timer for compatibility
    DUALTIMER_Start(DUALTIMER1_0);
}

void vMBPortTimersDisable(void) {
    DUALTIMER_Stop(DUALTIMER1_0);
}

void xMBRTUTimerT35Expired(void) {
    if(DUALTIMER_GetIntStatus(DUALTIMER1_0))
	{
		DUALTIMER_IntClear(DUALTIMER1_0);
        //printf("tim1 expired eRcvState = %d\r\n", eRcvState);
        switch (eRcvState) {
        /* Timer t35 expired. Startup phase is finished. */
        case STATE_RX_INIT:
            //printf("STATE_RX_INIT: Startup phase finished\r\n");
            break;

        /*  A frame was received and t35 expired. Notify the listener that
            a new frame was received. */
        case STATE_RX_RCV:
            //printf("STATE_RX_RCV: Frame received, setting mb_state_rtu_finish=true\r\n");
            mb_state_rtu_finish = true;
            break;

        /* An error occured while receiving the frame. */
        case STATE_RX_ERROR:
            //printf("STATE_RX_ERROR: Frame reception error\r\n");
            break;

        /* Function called in an illegal state. */
        default:
            //printf("WARNING: Timer expired in unexpected state: %d\r\n", eRcvState);
            break;
        }
        vMBPortTimersDisable();
        eRcvState = STATE_RX_IDLE;
    }
	if(DUALTIMER_GetIntStatus(DUALTIMER1_1))
	{
		DUALTIMER_IntClear(DUALTIMER1_1);
	}
}
