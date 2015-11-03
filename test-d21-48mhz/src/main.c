/**
* \file
*
* \brief Empty user application template
*
*/

/**
* \mainpage User Application template doxygen documentation
*
* \par Empty user application template
*
* Bare minimum empty user application template
*
* \par Content
*
* -# Include the ASF header files (through asf.h)
* -# Minimal main function that starts with a call to system_init()
* -# "Insert application code here" comment
*
*/

/*
* Include header files for all drivers that have been imported from
* Atmel Software Framework (ASF).
*/
/*
* Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
*/
#include <asf.h>

#define USE_ASF
//#define USE_DIRECT_REGISTERS


#define STDIO_HW		SERCOM0
#define STDIO_BAUD 		115200
#define STDIO_MUX 		USART_RX_1_TX_0_XCK_1
#define STDIO_PAD0		PINMUX_PA04D_SERCOM0_PAD0
#define STDIO_PAD1		PINMUX_PA05D_SERCOM0_PAD1
#define STDIO_PAD2		PINMUX_UNUSED
#define STDIO_PAD3		PINMUX_UNUSED

#define LED_PIN			PIN_PA27
#define LED_PIN_CONFIG	{ PORT_PIN_DIR_OUTPUT, PORT_PIN_PULL_NONE, false }


static struct usart_module usart_instance;




#ifdef USE_DIRECT_REGISTERS

void clock_48mhz(void) {
	// http://www.at91.com/discussions/viewtopic.php/f,31/t,22135.html
	uint32_t caldata[4];
	uint32_t dfll_fine_cal;
	uint32_t dfll_coarse_cal;
	uint32_t osc32_cal;

	SystemInit();

	/**** read some cal data from NVRAM ****/
	caldata[0] = *((uint32_t *)(0x806020));
	caldata[1] = *((uint32_t *)(0x806024));
	caldata[2] = *((uint32_t *)(0x806028));
	caldata[3] = *((uint32_t *)(0x80602C));
	dfll_coarse_cal = ((caldata[2] & 0xFC000000) >> 26);
	dfll_fine_cal = (caldata[3] & 0x3F);
	osc32_cal = ((caldata[1] & (0x7F << 6)) >> 6);

	// Clock 0 Output (for debug)
	PORT->Group[0].PINCFG[14].bit.PMUXEN = 1;     // allow pmux to set pin configuration
	PORT->Group[0].PMUX[7].bit.PMUXE = 7;        // PB14 = gclk 0

	// Clock 1 Output (for debug)
	PORT->Group[0].PINCFG[15].bit.PMUXEN = 1;     // allow pmux to set pin configuration
	PORT->Group[0].PMUX[7].bit.PMUXO = 7;        // PB15 = gclk 1



	/****** set up gclk 1 as a 32K reference for the DFLL ******/

	#undef ENABLE
	//32K source : enable and calibrate
	SYSCTRL->OSC32K.bit.ENABLE = 1;
	SYSCTRL->OSC32K.bit.EN32K = 1;
	SYSCTRL->OSC32K.bit.CALIB = osc32_cal;
	#define ENABLE    1

	// gclk 1   : Divide by 1
	GCLK->GENDIV.reg = 1 | (1 << 8);

	// gclk 1  : Enable, Output to IO, Use 32K source
	GCLK->GENCTRL.reg = 1 | (GCLK_GENCTRL_OE) | (GCLK_GENCTRL_GENEN) | (4 << GCLK_GENCTRL_SRC_Pos) ;

	// Clock Multiplexer : Send gclk1 output to DFLL ref input
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | (1 << GCLK_CLKCTRL_GEN_Pos) | (0x00 << GCLK_CLKCTRL_ID_Pos);


	/***** initialise the DFLL ******/

	// 48M Source  : errata fix
	SYSCTRL->DFLLCTRL.bit.ONDEMAND = 0 ;

	// 48M Source  : initialise value
	SYSCTRL->DFLLVAL.reg = 0x27F;

	// 48M Source  : set seek parameters and mul factor (32K * 1464 = 48MHz)
	SYSCTRL->DFLLMUL.reg = ( 2 << 26) | (2 << 16) | (1464);

	// 48M Source  : enable in close loop mode
	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE | SYSCTRL_DFLLCTRL_MODE;  // closed loop

	/*** wait for the DFLL to lock ****/
	while((SYSCTRL->PCLKSR.bit.DFLLLCKC == 0) || (SYSCTRL->PCLKSR.bit.DFLLLCKC == 0));

	/*****  before setting core to 48MHz, increase flash wait states *****/
	NVMCTRL->CTRLB.bit.RWS = 2;

	// gclk 0  : Enable, Output to IO, Use DFLL source
	GCLK->GENCTRL.reg = 0 | (GCLK_GENCTRL_OE) | (GCLK_GENCTRL_GENEN) | (7 << GCLK_GENCTRL_SRC_Pos) ;
}

#endif


void cph_stdio_init(void) {

	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);

	config_usart.baudrate = STDIO_BAUD;
	config_usart.mux_setting = STDIO_MUX;
	config_usart.pinmux_pad0 = STDIO_PAD0;
	config_usart.pinmux_pad1 = STDIO_PAD1;
	config_usart.pinmux_pad2 = STDIO_PAD2;
	config_usart.pinmux_pad3 = STDIO_PAD3;

	stdio_serial_init(&usart_instance, STDIO_HW, &config_usart);
	usart_enable(&usart_instance);
}

int main (void)
{
	uint32_t counter = 0;


#ifdef USE_ASF	
	system_init();
#endif

#ifdef USE_DIRECT_REGISTERS
	clock_48mhz();
#endif

	delay_init();
	
	cph_stdio_init();
	
	struct port_config config_port = LED_PIN_CONFIG;
	port_pin_set_config(LED_PIN, &config_port);
	
	//port_pin_toggle_output_level(LED_PIN);
	port_pin_set_output_level(LED_PIN, false);

	for (int i=0;i<50;i++) {
		port_pin_toggle_output_level(LED_PIN);
		delay_ms(100);
	}
	
	printf("TEST D21 48MHz\r\n");
	printf("CPU FREQ: %lu\r\n", system_gclk_gen_get_hz(0));
	
	while(1) {
		printf("%08x\r\n", counter++);
		delay_ms(500);
		port_pin_toggle_output_level(LED_PIN);
	}

}
