#define SYSCLK 72000000L
#define BAUDRATE 115200L
#define SARCLK 18000000L

#define VDD 3.3035

#define CHARS_PER_LINE 16
#define LCD_RS P1_7
#define LCD_E P2_0
#define LCD_D4 P1_3
#define LCD_D5 P1_2
#define LCD_D6 P1_1
#define LCD_D7 P1_0

char _c51_external_startup (void) {
	SFRPAGE = 0x00;
	WDTCN = 0xDE;
	WDTCN = 0xAD;

	VDM0CN = 0x80;
	RSTSRC = 0x02|0x04;

	#if (SYSCLK == 48000000L)
		SFRPAGE = 0x10;
		PFE0CN = 0x10;
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN = 0x20;
		SFRPAGE = 0x00;
	#endif

	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif

	P0MDOUT |= 0x10;
	XBR0 = 0x01;
	XBR1 = 0X00;
	XBR2 = 0x40;

	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 1 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif

	SCON0 = 0x10;
	TH1 = 0x100 - (SYSCLK/BAUDRATE/(2L*12L));
	TL1 = TH1;
	TMOD &= 0x0F;
	TMOD |= 0x20;
	TR1 = 1;
	TI = 1;

	return 0;
}

void adc_init (void) {
	SFRPAGE = 0x00;
	ADEN = 0;

	ADC0CN1 = (0x2 << 6)			// 0x0: 10-bit, 0x1: 12-bit, 0x2: 14-bit.
		| (0x0 << 3)			// 0x0: No shift. 0x1: Shift right 1 bit. 0x2: Shift right 2 bits. 0x3: Shift right 3 bits.
		| (0x0 << 0);			// Accumulate n conversions: 0x0: 1, 0x1:4, 0x2:8, 0x3:16, 0x4:32.

	ADC0CF0 = ((SYSCLK/SARCLK) << 3)	// SAR Clock Divider. Max is 18MHz. Fsarclk = (Fadcclk) / (ADSC + 1).
		| (0x0 << 2);			// 0:SYSCLK ADCCLK = SYSCLK. 1:HFOSC0 ADCCLK = HFOSC0.

	ADC0CF1 = (0 << 7)			// 0: Disable low power mode. 1: Enable low power mode.
		| (0x1E << 0);			// Conversion Tracking Time. Tadtk = ADTK / (Fsarclk).

	ADC0CN0 = (0x0 << 7)			// ADEN. 0: Disable ADC0. 1: Enable ADC0.
		| (0x0 << 6)			// IPOEN. 0: Keep ADC powered on when ADEN is 1. 1: Power down when ADC is idle.
		| (0x0 << 5)			// ADINT. Set by hardware upon completion of a data conversion. Must be cleared by firmware.
		| (0x0 << 4)			// ADBUSY. Writing 1 to this bit initiates an ADC conversion when ADCM = 000. This bit should not be polled to indicate when a conversion is complete. Instead, the ADINT bit should be used when polling for conversion completion.
		| (0x0 << 3)			// ADWINT. Set by hardware when the contents of ADC0H:ADC0L fall within the window specified by ADC0GTH:ADC0GTL and ADC0LTH:ADC0LTL. Can trigger an interrupt. Must be cleared by firmware.
		| (0x0 << 2)			// ADGN (Gain Control). 0x0: PGA gain=1. 0x1: PGA gain=0.75. 0x2: PGA gain=0.5. 0x3: PGA gain=0.25.
		| (0x0 << 0);			// TEMPE. 0: Disable the Temperature Sensor. 1: Enable the Temperature Sensor.

	ADC0CF2 = (0x0 << 7)			// GNDSL. 0: reference is the GND pin. 1: reference is the AGND pin.
		| (0x1 << 5)			// REFSL. 0x0: VREF pin (external or on-chip). 0x1: VDD pin. 0x2: 1.8V. 0x3: internal voltage reference.
		| (0x1F << 0);			// ADPWR. Power Up Delay Time. Tpwrtime = ((4 * (ADPWR + 1)) + 2) / (Fadcclk).

	ADC0CN2 = (0x0 << 7)			// PACEN. 0x0: The ADC accumulator is over-written. 0x1: The ADC accumulator adds to results.
		| (0x0 << 0);			// ADCM. 0x0: ADBUSY, 0x1: TIMER0, 0x2: TIMER2, 0x3: TIMER3, 0x4: CNVSTR, 0x5: CEX5, 0x6: TIMER4, 0x7: TIMER5, 0x8: CLU0, 0x9: CLU1, 0xA: CLU2, 0xB: CLU3.

	ADEN=1;
}

void sleep_us(unsigned char us) {
	unsigned char i;

	CKCON0 |= 0b01000000;

	TMR3RL = -SYSCLK/1000000L;
	TMR3 = TMR3RL;

	TMR3CN0 = 0x04;
	for (i = 0; i < us; ++i) {
		while (!(TMR3CN0 & 0x80));
		TMR3CN0 &= ~0x80;
	}
	TMR3CN0 = 0;
}

void sleep(unsigned int ms) {
	unsigned int i, j;
	for(i = 0; i < ms; ++i)
		for (j = 0; j < 4; ++j)
			sleep_us(250);
}

void adc_pin_init(unsigned char portno, unsigned char pinno) {
	unsigned char mask;

	mask = 1 << pinno;

	SFRPAGE = 0x20;
	switch (portno) {
		case 0:
			P0MDIN &= (~mask);
			P0SKIP |= mask;
		break;
		case 1:
			P1MDIN &= (~mask);
			P1SKIP |= mask;
		break;
		case 2:
			P2MDIN &= (~mask);
			P2SKIP |= mask;
		break;
		default:
		break;
	}
	SFRPAGE = 0x00;
}

unsigned int adc_value(unsigned char pin) {
	ADC0MX = pin;
	ADINT = 0;
	ADBUSY = 1;
	while (!ADINT);
	return ADC0;
}

float adc_volts(unsigned char pin) {
	 return VDD * adc_value(pin) / 0b0011111111111111;
}

void lcd_pulse (void) {
	LCD_E = 1;
	sleep_us(40);
	LCD_E = 0;
}

void lcd_byte (unsigned char x) {
	ACC = x;
	LCD_D7 = ACC_7;
	LCD_D6 = ACC_6;
	LCD_D5 = ACC_5;
	LCD_D4 = ACC_4;
	lcd_pulse();
	sleep_us(40);
	ACC = x;
	LCD_D7 = ACC_3;
	LCD_D6 = ACC_2;
	LCD_D5 = ACC_1;
	LCD_D4 = ACC_0;
	lcd_pulse();
}

void lcd_write_data(unsigned char x) {
	LCD_RS = 1;
	lcd_byte(x);
	sleep(2);
}

void lcd_write_command(unsigned char x) {
	LCD_RS = 0;
	lcd_byte(x);
	sleep(5);
}

void lcd_init(void) {
	LCD_E=0;
	sleep(20);
	lcd_write_command(0x33);
	lcd_write_command(0x33);
	lcd_write_command(0x32);

	/* Configure the LCD. */
	lcd_write_command(0x28);
	lcd_write_command(0x0c);
	lcd_write_command(0x01);
	sleep(20);
}

void lcd_print(char *string, unsigned char line, bit clear) {
	unsigned int i;

	lcd_write_command(line == 1 ? 0x80 : 0xc0);
	sleep(5);
	for (i = 0; string[i] != 0; ++i)
		lcd_write_data(string[i]);
	if (clear)
		for(; i < CHARS_PER_LINE; ++i)
			lcd_write_data(' ');
}
