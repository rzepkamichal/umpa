/*
 * AVR_bluetooth.c
 *
 * Created: 22.04.2020 21:02:23
 * Author : dmusial98
 
 - PD2 (INT0) przerwanie kontaktronu na narastajacym zboczu
 - PC0 dioda pulsuje przy przyjsciu przerwania
 - transmisja USART na 9600 bitow na sekunde
 - czestotliwosc 16 MHz
 - wysylanie czasu w milisekundach miedzy pojedynczymi impulsami z kontaktronu
 */ 

#define FOSC 16000000UL
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1

#include <avr/io.h>
#include <avr/interrupt.h>

#define KONTAKTRON_KIERUNEK DDRD
#define KONTAKTRON_PORTY PORTD

volatile uint64_t interwal = 0;

void USART_inicjalizacja()
{
	UBRRH = (unsigned char) (MYUBRR >> 8); 
	UBRRL = (unsigned char) (MYUBRR);
	
	UCSRB = (1 << TXEN);  //zalaczone nadawanie
	UCSRC =  (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);; // 8 bitow danych 1 bit stopu bez sprawdzania parzystosci
}

void USART_transmisja(uint64_t data)
{
	for(int i = 7; i >= 0; i--)
	{
		while(!(UCSRA & (1 << UDRE)));
		
		UDR = (data >> i * 8); //wyslanie calej 64-bitowej danej
	}
}

void kontaktron_inicjalizacja() //Na pinie PD2 zewnetrzne przerwanie od narastajacego zbocza kontaktronu
{
	KONTAKTRON_KIERUNEK &= ~(1 << PD2); //na PD2 wejscie
	KONTAKTRON_PORTY |= (1 << PD2); //rezystor podciagajacy na PD2
	
	MCUCR |= ((1 << ISC01) | (1 << ISC00)); //przerwanie na narastajacym zboczu PD2 (INT0)
	GICR |= (1 << INT0); //wlaczanie przerwania na PD2
}

void licznik2_inicjalizacja()
{
	TCCR2 |= (1 << WGM21 | 1 << CS22 | 1 << CS20); //tryb CTC i clk/128 
	OCR2 = 125; // wartosc do wywolania przerwania (odliczenie 1 milisekundy)
	TIMSK |= (1 << OCIE2); //odblokowanie przerwania w trybie compare match interrupt
}

ISR(INT0_vect) //obsluga przerwania na przycisku PD2 -> stan niski
{
	PORTC ^= (1 << PINC0);
	USART_transmisja(interwal);
	interwal = 0;
}

ISR(TIMER2_COMP_vect) //odliczenie 1 ms
{
	interwal++;
}

int main(void)
{
	kontaktron_inicjalizacja();
	DDRC |= 0x01; //wyjscie na porcie PC0
	PORTC |= 0x01; //stan wysoki na PC0
	USART_inicjalizacja();
	licznik2_inicjalizacja();
	sei();
	
    while (1) 
    {
    }
}

