/*
 * LWM_MSSY.c
 *
 * Created: 6.4.2017 15:42:46
 * Author : xramos00
 */ 

#include <avr/io.h>
/*- Includes ---------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "hal.h"
#include "phy.h"
#include "sys.h"
#include "nwk.h"
#include "sysTimer.h"
#include "halBoard.h"
#include "halUart.h"
#include "main.h"

/*- Definitions ------------------------------------------------------------*/
#ifdef NWK_ENABLE_SECURITY
#define APP_BUFFER_SIZE     (NWK_MAX_PAYLOAD_SIZE - NWK_SECURITY_MIC_SIZE)
#else
#define APP_BUFFER_SIZE     NWK_MAX_PAYLOAD_SIZE
#endif

/*- Types ------------------------------------------------------------------*/
typedef enum AppState_t
{
	APP_STATE_INITIAL,
	APP_STATE_IDLE,
} AppState_t;

/*- Prototypes -------------------------------------------------------------*/
static void appSendData(void);

/*- Variables --------------------------------------------------------------*/
static AppState_t appState = APP_STATE_INITIAL;
static SYS_Timer_t appTimer;
static NWK_DataReq_t appDataReq;
static bool appDataReqBusy = false;
static uint8_t appDataReqBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBufferPtr = 0;

/*- Implementations --------------------------------------------------------*/

/*************************************************************************//**
*****************************************************************************/
static void appDataConf(NWK_DataReq_t *req)
{
appDataReqBusy = false;
(void)req;
}

/*************************************************************************//**
*****************************************************************************/
static void appSendData(void)
{
if (appDataReqBusy || 0 == appUartBufferPtr)
return;

memcpy(appDataReqBuffer, appUartBuffer, appUartBufferPtr);

appDataReq.dstAddr = 1-APP_ADDR;
appDataReq.dstEndpoint = APP_ENDPOINT;
appDataReq.srcEndpoint = APP_ENDPOINT;
appDataReq.options = NWK_OPT_ENABLE_SECURITY;
appDataReq.data = appDataReqBuffer;
appDataReq.size = appUartBufferPtr;
appDataReq.confirm = appDataConf;
NWK_DataReq(&appDataReq);

appUartBufferPtr = 0;
appDataReqBusy = true;
}

static void moje_SendData(uint16_t srcAddr, uint8_t * data, uint8_t size)
{
	if (appDataReqBusy)
	return;

	

	appDataReq.dstAddr = srcAddr;
	appDataReq.dstEndpoint = APP_ENDPOINT;
	appDataReq.srcEndpoint = APP_ENDPOINT;
	appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	appDataReq.data = data;
	appDataReq.size = size;
	appDataReq.confirm = appDataConf;
	NWK_DataReq(&appDataReq);
	
	appDataReqBusy = true;
}

/*************************************************************************//**
*****************************************************************************/
void HAL_UartBytesReceived(uint16_t bytes)
{
for (uint16_t i = 0; i < bytes; i++)
{
uint8_t byte = HAL_UartReadByte();

if (appUartBufferPtr == sizeof(appUartBuffer))
appSendData();

if (appUartBufferPtr < sizeof(appUartBuffer))
appUartBuffer[appUartBufferPtr++] = byte;
}

SYS_TimerStop(&appTimer);
SYS_TimerStart(&appTimer);
}

/*************************************************************************//**
*****************************************************************************/
static void appTimerHandler(SYS_Timer_t *timer)
{
//appSendData();
(void)timer;
}

/*************************************************************************//**
*****************************************************************************/
static bool appDataInd(NWK_DataInd_t *ind)
{
	uint16_t tmpSrcAddr = ind->srcAddr;
	moje_SendData(tmpSrcAddr,(uint8_t*) "OK", 2);
for (uint8_t i = 0; i < ind->size; i++)
HAL_UartWriteByte(ind->data[i]);
return true;
}

/*************************************************************************//**
*****************************************************************************/

static bool printFrame(NWK_DataInd_t *ind)
{
	HAL_UartWriteByte(nwkIb.addr);
	HAL_UartWriteByte(ind->dstAddr);
	return true;
}

static void appInit(void)
{
NWK_SetAddr(APP_ADDR);
NWK_SetPanId(APP_PANID);
PHY_SetChannel(APP_CHANNEL);
#ifdef PHY_AT86RF212
PHY_SetBand(APP_BAND);
PHY_SetModulation(APP_MODULATION);
#endif
PHY_SetRxState(true);

HAL_BoardInit();

appTimer.interval = APP_FLUSH_TIMER_INTERVAL;
appTimer.mode = SYS_TIMER_INTERVAL_MODE;
appTimer.handler = appTimerHandler;
}

/*************************************************************************//**
*****************************************************************************/
static void APP_TaskHandler(void)
{
switch (appState)
{
case APP_STATE_INITIAL:
{
appInit();
appState = APP_STATE_IDLE;
} break;

case APP_STATE_IDLE:
	
break;

default:
break;
}
}

/* VYTVORENE V RAMCI PROJEKTU - ZACIATOK */
void sendString( char * str)
{
	// funkcia vypise cez UART vsetky znaky retazca z pamate kam ukazuje char * str
	// funkcia detekuje koniec retaca podla znaku '\0'
	HAL_UartWriteByte(*str);
	while(++str != '\0')
	{
		HAL_UartWriteByte(*str);
	}
}

// funkcie na vypis znaku a retazca znakov z cviceni
void UART_SendChar(uint8_t data)
{
	while ( !( UCSR1A & (1<<UDRE0)) )
	;
	UDR1 = data;
}

void UART_SendString(char *text){
	while (*text != 0x00)
	{
		UART_SendChar(*text);
		text++;
	}
}
/* VYTVORENE V RAMCI PROJEKTU - KONIEC */

/*************************************************************************//**
*****************************************************************************/
int main(void)
{
SYS_Init();
HAL_UartInit(38400);

/* VYTVORENE V RAMCI PROJEKTU - ZACIATOK */

// odoslanie CSV hlavièky cez UART
UART_SendString(HLAVICKA);

for (int i = 0; i < NWK_ENDPOINTS_AMOUNT; i++)
{
	// otvorenie vsetkych enpointov pre prijimanie komunikacie
	NWK_OpenEndpoint(i, printFrame);
}
/* VYTVORENE V RAMCI PROJEKTU - KONIEC */

while (1)
{
SYS_TaskHandler();
HAL_UartTaskHandler();
APP_TaskHandler();
}
}
