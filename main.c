#ifndef F_CPU
#define F_CPU 3333333UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"


#define TRIG_PIN 7   // PA7 (ultraäänen trig-pinni)
#define ECHO_PIN 6   // PA6 (ultraäänen echo-pinni)
#define BUZZER_PIN PIN3_bm  // PA3 (kaiuttimen pinni)

unsigned int duration; // ultraäänisensorin pulssin aika
float distance; // ultraäänisensorin mittaama etäisyys


// kaiuttimen määrittely
void buzzer_init(void) {
    PORTA.DIRSET = BUZZER_PIN; // PA3 pinni ulostuloksi
}
// päälle
void buzzer_on(void) {
    PORTA.OUTSET = BUZZER_PIN;
}
// pois 
void buzzer_off(void) {
    PORTA.OUTCLR = BUZZER_PIN;
}




// ultraäänisensorin määrittely
void ultrasonic_init(void) {
    PORTA.DIRSET = (1 << TRIG_PIN);   // trig-pinni ulostuloksi
    PORTA.DIRCLR = (1 << ECHO_PIN);   // echo-pinni sisääntuloksi
    TCA0.SINGLE.CNT = 0; // ajastimen nollaus
}

// etäisyyden mittaaminen ultraäänisensorilla
unsigned int measure_distance(void) {
    // lähtevän pulssin lähetys:
    PORTA.OUTCLR = (1 << TRIG_PIN);  //varmistetaan että TRIG on alhaalla (LOW)
    _delay_us(2);
    PORTA.OUTSET = (1 << TRIG_PIN); // asetetaan TRIG HIGH
    _delay_us(10);
    PORTA.OUTCLR = (1 << TRIG_PIN); // asetetaan TRIG takaisin LOW

    
        
    unsigned int timeout = 100000; // aikakatkaisu, mikäli ECHO ei palaudu takaisin lähettimeen
    while (!(PORTA.IN & (1 << ECHO_PIN))) {  // odotetaan että ECHO-pin nosuee HIGH
        if (--timeout == 0) return 0; // mikäli pulssia ei saada takaisin lähettimeen
    }
    unsigned int start = TCA0.SINGLE.CNT; // ajastimen arvo lopetushetkellä

    timeout = 100000; // nollataan
    while (PORTA.IN & (1 << ECHO_PIN)) { // odotetaan että ECHO-pin takaisin LOW
        if (--timeout == 0) return 0; // mikäli pulssia ei saada takaisin lähettimeen
    }
    unsigned int end = TCA0.SINGLE.CNT; // ajastimen arvo lopetushetkellä

    if (end < start) {
        return (0xFFFF - start + end); // jos ajastin ylivuotaa (0xFFFF = 65535)
    } else {
        return end - start; // pulssin keston
    }
}

// mikro servoa:
// Alustaa ajastimen TCA0 tuottamaan PWM-signaalia 
void TCA0_init(void) {
    PORTA.DIRSET = PIN2_bm;  // PA2 on pwm (Pulse Width Modulation) ulostulo WO2

    // Route WO0 to PORTA
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;

    
    // PWM single slope, enable WO0
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP2EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;

    
    // 20 ms period
    TCA0.SINGLE.PER = 1041;

    // 1.5 ms pulse = center
    TCA0.SINGLE.CMP2 = 78;

    // Enable timer with prescaler 64
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;

}



int main(void) {
    buzzer_init(); // alustetaan kaiutin
    TCA0_init(); // alustetaan ajastin
    ultrasonic_init(); // alustetaan ultraäänisensori
    lcd_init(LCD_DISP_ON); // alustetaan LCD-näyttö
    
    
    buzzer_off(); // kaiutin pois päältä
    _delay_ms(500);  // Anturi valmiiksi

    char buffer[17]; // merkkijono


while (1) {
    unsigned long sum = 0; // mittausten summa
    unsigned int valid = 0; // onnistuneet mittaukset
    
    // viisi mittausta
    for (int i = 0; i < 5; i++) {
        unsigned int d = measure_distance();
        if (d > 0) {
            sum += d;
            valid++;
        }
        _delay_ms(100);
    }

    // ei onnistuneita mittauksia
    if (valid == 0) {
        lcd_gotoxy(0, 0);
        lcd_puts("sensori ei toimi");
        lcd_gotoxy(0, 1);
        lcd_puts("                ");
    } else {
        //lasketaan kesto
        duration = sum / valid;

        float time_us = duration * 0.4; // ajastimen laskuriarvo mikrosekunneiksi
        distance = time_us / 58.0; //  // etäisyys senteissä

    }
    
    if (distance > 0.7){
        buzzer_off();
        lcd_clrscr();
        lcd_puts("Kohdetta ei");
        lcd_gotoxy(0, 1);
        lcd_puts("havaittu");
    }
    else if ((distance >= 0.3) && (distance <= 0.6)){
        buzzer_off();
        lcd_clrscr();
        lcd_puts("kohde on kaukana");

    }
    else if ((distance <= 0.2) && (distance >= 0.1)){
        buzzer_on();
        lcd_clrscr();
        lcd_puts("kohde on lahella");
        TCA0.SINGLE.CMP2 = 104;
        _delay_ms(200);
        TCA0.SINGLE.CMP2 = 52;
        _delay_ms(220);
        buzzer_off();
        _delay_ms(500);
    } 
    else if (distance <= 0.1 ){
        int c = 0;
        buzzer_on();
        
        lcd_clrscr();
        lcd_puts("kontaktissa");
        
        while(c < 5){
            TCA0.SINGLE.CMP2 = 104;
        _delay_ms(200);
        TCA0.SINGLE.CMP2 = 52;
        _delay_ms(200);
        c++;
        }                   
    }
    _delay_ms(1000);
}
}
