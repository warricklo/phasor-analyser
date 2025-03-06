#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>

#include "util.h"

#define OFFSET_PERIOD 175
#define OFFSET_PHASE -125

float get_period();
float get_peak_voltage();
float get_phase_shift(float period);

int main(void) {
	float period, frequency, voltage_peak, voltage_peak_rms, phase;
	char buf[17];

	/* Initialise ADC. */
	adc_pin_init(1, 4);
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
		/* Get period and frequency. */
		period = get_period();
		frequency = 1000/period;

		/* Get peak voltage. */
		voltage_peak = get_peak_voltage();
		voltage_peak_rms = 1.414213562 * voltage_peak;

		phase = get_phase_shift(period);
		printf("%f\r\n", phase);

		// sprintf(buf, "T: %6.4f ms", period);
		sprintf(buf, "V: %6.4f V", voltage_peak);
		// sprintf(buf, "V: %6.4f V RMS", voltage_peak_rms);
		lcd_print(buf, 1, 1);
		sprintf(buf, "f: %6.4f Hz", frequency);
		lcd_print(buf, 2, 1);
	}

	return 0;
}

float get_period() {
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

	return (float)24000 * (unsigned int)(256*TH0+TL0+OFFSET_PERIOD) / SYSCLK;
}

float get_peak_voltage() {
	float voltage, voltage_peak;
	voltage_peak = 0;
	while (adc_volts(QFP32_MUX_P2_6) <= 0.005);
	while ((voltage = adc_volts(QFP32_MUX_P2_6)) >= 0.001) {
		if (voltage > voltage_peak) {
			voltage_peak = voltage;
		}
	}

	return voltage_peak;
}

float get_phase_shift(float period) {
	int sign;
	/* Stop and reset timer 0. */
	TR0 = 0;
	TH0 = 0;
	TL0 = 0;
	/* Wait for both signals to be zero and start timer 0. */
	while ((adc_volts(QFP32_MUX_P1_4) >= 0.001) || (adc_volts(QFP32_MUX_P2_6) >= 0.001));
	while ((adc_volts(QFP32_MUX_P1_4) <= 0.005) && (adc_volts(QFP32_MUX_P2_6) <= 0.005));
	TR0 = 1;
	sign = (adc_volts(QFP32_MUX_P1_4) <= 0.005) ? 1 : -1;
	/* Run until both signals are not zero. */
	while ((adc_volts(QFP32_MUX_P1_4) <= 0.005) || (adc_volts(QFP32_MUX_P2_6) <= 0.005));
	TR0 = 0;
	return (float)360 * 12000 * (unsigned)ABS(256*TH0+TL0+OFFSET_PHASE) / SYSCLK / period * sign;
}
