/*
 * @file utility.c
 *
 * @date Jul 24, 2017
 * @author hamster
 *
 *  Utility functions
 *
 */

#include <chrono>
#include "utility.h"
#include "FrameBuffer.h"
#include "EnginePanic.h"
#include "fonts/Monaco9.h"
#include <filesystem>


#ifdef DC801_EMBEDDED

#else
#include "sdk/shim/shim_timer.h"
#include "sdk/shim/shim_pwm.h"
#include "modules/adc.h"
#include "modules/led.h"
#endif


#define EE_R1 3900
#define EE_R2 10000
#define EE_R3 15000
#define EE_R4 4999
#define EE_TOTR (EE_R1 + EE_R2 + EE_R3 + EE_R4)
#define EE_VOLT(X) (getVccMillivolts() * (X) / EE_TOTR)

APP_TIMER_DEF(sysTickID);
APP_PWM_INSTANCE(PWM1, 1);
APP_TIMER_DEF(morseID);
volatile static uint32_t systick = 0;
bool morse_running = false;

/**
 * Start the local time reference
 * It's seeded at bootup from the user storage, if it exists
 * @param time
 */
void sysTickStart(void){
	systick = 0;
	app_timer_create(&sysTickID, APP_TIMER_MODE_REPEATED, sysTickHandler);
	app_timer_start(sysTickID, APP_TIMER_TICKS(1000), NULL);
}

/**
 * @return number of seconds since we started counting time
 */
uint32_t getSystick(void){
	return systick;
}

/**
 * Every second, update the systick handler
 * @param p_context
 */
void sysTickHandler(void * p_context){
	systick++;
}

uint8_t getButton(bool waitForLongPress) { return 0; }

//SOS RCVED BK MANY HOSTILES BK PLS TRNSMIT CODE 801801 WHEN RDY 4 SUPORT FN
//...  ---  ...      .-.  -.-.  ...-  .  -..      -...  -.-      --  .-  -.  -.--      ....  ---  ...  -  ..  .-..  .  ...      -...  -.-      .--.  .-..  ...      -  .-.  -.  ...  --  ..  -      -.-.  ---  -..  .      ---..  -----  .----  ---..  -----  .----      .--  ....  .  -.      .-.  -..  -.--      ....-      ...  ..-  .--.  ---  .-.  -      ..-.  -.
const uint8_t morseMessage[] = {1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t morseIdx =0;

uint8_t ls;
void morseTickHandler(void * p_context) {
	uint8_t s = morseMessage[morseIdx];

	if (s != ls){
		if (s)
			ledOn(LED_HAX);
		else
			ledOff(LED_HAX);
	}
	ls=s;

	morseIdx = ((morseIdx+1) % sizeof(morseMessage));
}

void morseInit(void ) {
	app_timer_create(&morseID, APP_TIMER_MODE_REPEATED, morseTickHandler);
}

void morseStart(void) {
	morseIdx=0;
	ls = 2;
	app_timer_start(morseID, APP_TIMER_TICKS(80), NULL);  //15WPM
	morse_running = true;
}

void morseStop(void) {
	app_timer_stop(morseID);
	ledOff(LED_HAX);
	morse_running = false;
}

bool morseGetRunning(void){
	return morse_running;
}

#if defined(GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#if defined(GCC)
#pragma GCC diagnostic pop
#endif

/**
 * Calculate the CRC on a chunk of memory
 * @param data
 * @param len
 * @return
 */
uint16_t calcCRC(uint8_t *data, uint8_t len, const uint16_t POLYNOM){
	uint16_t crc;
	uint8_t aux = 0;

	crc = 0x0000;

	while (aux < len){
		crc = crc16(crc, data[aux], POLYNOM);
		aux++;
	}

	return (crc);
}

/**
 * Calculate the crc16 of a value
 * @param crcValue
 * @param newByte
 * @return
 */
uint16_t crc16(uint16_t crcValue, uint8_t newByte, const uint16_t POLYNOM){
	uint8_t i;

	for (i = 0; i < 8; i++) {

		if (((crcValue & 0x8000) >> 8) ^ (newByte & 0x80)){
			crcValue = (crcValue << 1)  ^ POLYNOM;
		}else{
			crcValue = (crcValue << 1);
		}

		newByte <<= 1;
	}

	return crcValue;
}

#ifndef OVERFLOW
#define OVERFLOW ((uint32_t)(0xFFFFFFFF/32.768))
#endif

uint32_t millis_elapsed(uint32_t currentMillis, uint32_t previousMillis)
{
	if(currentMillis <= previousMillis)
	{
		return 0;
	}

	return(currentMillis - previousMillis);
}

uint32_t millis(void)
{
#ifdef DC801_EMBEDDED
	return(app_timer_cnt_get() / 32.768);
#else
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

void EEpwm_init() {
	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(5000L, 11);
	APP_ERROR_CHECK(app_pwm_init(&PWM1,&pwm1_cfg,NULL));
	app_pwm_enable(&PWM1);
}


void EEpwm_set(int r)
{
	r = r%101;
	app_pwm_channel_duty_set(&PWM1, 0, 100-r);
}

void EEget_milliVolts(int r, int *v1, int *v2, int *v3) {
	*v1 = EE_VOLT(EE_R4) * r / 100;
	*v2 = EE_VOLT(EE_R3 +EE_R4) * r / 100;
	*v3 = EE_VOLT(EE_R2 + EE_R3 + EE_R4) * r / 100;
}

//Turns hex 0x2305 to 2305
uint32_t hex2dec(uint32_t v) {
	uint32_t val = 0;
	const int tens[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
	for (int i=0; i<8; ++i)
		val += ((v >> (i*4)) & 0xF) * tens[i];
	return val;
}

void util_sd_error()
{
	//ENGINE_PANIC("SD Card Error\nCheck card and reboot");
	p_canvas()->clearScreen(COLOR_BLUE);
	p_canvas()->printMessage(
		"SD Card did not initialize properly.\n\
		Check Card and Reboot if you\n\
		want to use the SD Card to reflash\n\
		the ROM chip with a new mage.dat file.",
		Monaco9,
		COLOR_WHITE,
		32,
		32
	);
	p_canvas()->blt();
	nrf_delay_ms(5000);
}

void util_gfx_init()
{
	area_t area = { 0, 0, WIDTH, HEIGHT };
	p_canvas()->setTextArea(&area);

	p_canvas()->clearScreen(COLOR_BLACK);

	p_canvas()->blt();
}

//this creates one heap variable and one stack variable, and subtracts them
//to find the free ram where the function was called.
//it uses debug_print for output.
void check_ram_usage(void)
{
	uint8_t stack = 0;
	void * heap = malloc(1);
	debug_print("Stack Memory Address: 0x%x",(uint32_t)&stack);
	debug_print("Heap Memory Address:  0x%x", (uint32_t)heap);
	debug_print("Free Memory: %u", (uint32_t)(&stack - (uint8_t*)heap));
	//to prevent memory leaks:
	free(heap);
}