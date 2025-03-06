#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>

#include "util.h"

#define TIMER_OFFSET 175

int main(void) {
	unsigned int count;
	float period, frequency;
	char buf[17];

	/* Initialise ADC. */
	adc_pin_init(2, 6);
	adc_init();

	/* Initialise timer 0. */
	TMOD &= 0b11110000;
	TMOD |= 0b00000001;

	lcd_init();

	/* Clear the screen and print program info. */
	printf("\x1b[2J");
	printf("Phasor voltage detector\n"
		"File: %s\n"
		"Compiled: %s, %s\n\n",
		__FILE__, __DATE__, __TIME__);

	sprintf(buf, "Waiting for");
	lcd_print(buf, 1, 1);
	sprintf(buf, "signal...");
	lcd_print(buf, 2, 1);

	while (1) {
		/* Stop and reset timer 0. */
		TR0 = 0;
		TH0 = 0;
		TL0 = 0;
		/* Wait for rising edge and start timer 0. */
		while (adc_volts(QFP32_MUX_P2_6) >= 0.001);
		while (adc_volts(QFP32_MUX_P2_6) <= 0.005);
		TR0 = 1;
		while (adc_volts(QFP32_MUX_P2_6) >= 0.001);
		TR0 = 0;

		period = (float)24000 * (unsigned int)(256*TH0+TL0+TIMER_OFFSET) / SYSCLK;
		frequency = 1000/period;

		sprintf(buf, "T: %6.4f ms", period);
		lcd_print(buf, 1, 1);
		sprintf(buf, "f: %6.4f Hz", frequency);
		lcd_print(buf, 2, 1);
	}

	return 0;
}
