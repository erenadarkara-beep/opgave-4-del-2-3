#include <Arduino.h>
#include "ssd1306.h"
#include "i2c.h"
#include <msp430.h>
#include <msp430f5529.h>

volatile unsigned int adc_reading = 0;

void setup_pwm_timer(void) {
    TA1CTL = TASSEL_2 | MC_1 | TACLR;     // SMCLK som kilde, op-til mode, clear
    TA1CCR0 = 999;                        // Periode: 1000 cycles (1kHz ved 1MHz)
    TA1CCR1 = 500;                        // Start duty cycle: 50%
    TA1CCTL1 = OUTMOD_7;                  // Reset/set output mode
}

void setup_adc_trigger_timer(void) {
    TA0CTL = TASSEL_2 | MC_1 | TACLR;     // SMCLK, up mode, clear
    TA0CCR0 = 999;                        // 1ms interval ved 1MHz
    TA0CCTL0 = CCIE;                      // Enable compare interrupt
}

void configure_adc(void) {
    ADC12CTL0 &= ~ADC12ENC;               // Deaktiver for konfiguration
    ADC12CTL0 = ADC12SHT0_3 | ADC12ON;    // 32 cycle sample time, enable ADC
    ADC12CTL1 = ADC12SHP;                 // Sampling timer mode
    ADC12MCTL0 = ADC12INCH_0;             // Input channel A0 (P6.0)
    ADC12IE = ADC12IE0;                   // Enable interrupt for memory 0
    P6SEL |= BIT0;                        // Analog funktion på P6.0
    P6REN &= ~BIT0;                       // Ingen pull-resistor
    ADC12CTL0 |= ADC12ENC;                // Aktiver konvertering
}

// Timer for ADC-trigger
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0_Handler(void) {
    ADC12CTL0 |= ADC12SC;                 // Start ADC konvertering
}

// ADC konverterings færdig interrupt
#pragma vector=ADC12_VECTOR
__interrupt void ADC_Conversion_Handler(void) {
    unsigned int interrupt_vector = ADC12IV;
    if (interrupt_vector == 6) {          // ADC12IFG0 - memory 0 klar
        adc_reading = ADC12MEM0;
        // Opdater PWM duty cycle baseret på ADC værdi
        TA1CCR1 = ((unsigned long)adc_reading * TA1CCR0) / 4095;
    }
}

unsigned int duty_cycle_percent = 0;

int main() {
    WDTCTL = WDTPW | WDTHOLD;             // Stop watchdog timer

    // Display initialisering
    i2c_init();
    ssd1306_init();
    ssd1306_clearDisplay();

    // Hardware opsætning
    setup_pwm_timer();
    setup_adc_trigger_timer();
    configure_adc();

    // Konfigurer PWM output pin
    P2DIR |= BIT0;                        // P2.0 som output
    P2SEL |= BIT0;                        // Secondary function (PWM)

    __enable_interrupt();

    char display_text[20];

    while(1) {
        // Beregn duty cycle i procent
        duty_cycle_percent = ((unsigned long)TA1CCR1 * 100) / TA1CCR0;

        // Opdater display med måledata
        sprintf(display_text, "Timer:%u", TA1R);
        ssd1306_printText(0, 0, display_text);

        sprintf(display_text, "Compare:%03u", TA1CCR1);
        ssd1306_printText(0, 1, display_text);

        sprintf(display_text, "Cycle:%03u%%", duty_cycle_percent);
        ssd1306_printText(0, 2, display_text);

        sprintf(display_text, "Sensor:%04u", adc_reading);
        ssd1306_printText(0, 3, display_text);

        // Vent 200ms før næste opdatering
        __delay_cycles(200000);
    }
}
