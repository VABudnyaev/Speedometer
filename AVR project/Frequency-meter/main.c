/*
 * Frequency-meter.c
 *
 * Created: 10.02.2022 19:09:19
 * Author : vadim
 */ 

# define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int r1_1000 = 0;
int r2_100 = 0;
int r3_10 = 0;
int r4_1 = 0;

int seconds = 0;
int z = 1;

void digits(int digit);
void number (unsigned int vse_chislo);

unsigned int count_interrupts = 0;
unsigned long count_pulses = 0;

int count_10 = 0;
double average_time = 0;

void digits(int digit)
{
	switch(digit)
	{
		case 0: PORTD &= ~(1<<6);
		PORTC |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
		PORTD |= (1<<4) | (1<<5);
		break;
		
		case 1: PORTC &= ~((1<<0) | (1<<3));
		PORTC |= (1<<1) | (1<<2);
		PORTD &= ~((1<<4) | (1<<5) | (1<<6));
		break;
		
		case 2: PORTC &= ~(1<<2);
		PORTC |= (1<<0) | (1<<1) | (1<<3);
		PORTD |= (1<<4) | (1<<6);
		PORTD &= ~(1<<5);
		break;
		
		case 3: PORTD &= ~((1<<4) | (1<<5));
		PORTC |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
		PORTD |= (1<<6);
		break;
		
		case 4: PORTC &= ~((1<<0) | (1<<3));
		PORTC |= (1<<1) | (1<<2);
		PORTD |= (1<<5) | (1<<6);
		PORTD &= ~(1<<4);
		break;
		
		case 5: PORTC &= ~(1<<1);
		PORTC |= (1<<0) | (1<<2) | (1<<3);
		PORTD |= (1<<5) | (1<<6);
		PORTD &= ~(1<<4);
		break;
		
		case 6: PORTC &= ~(1<<1);
		PORTC |= (1<<0) | (1<<2) | (1<<3);
		PORTD |= (1<<4) | (1<<5) | (1<<6);
		break;
		
		case 7: PORTC &= ~(1<<3);
		PORTC |= (1<<0) | (1<<1) | (1<<2);
		PORTD &= ~((1<<4) | (1<<5) | (1<<6));
		break;
		
		case 8: PORTC |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
		PORTD |= (1<<4) | (1<<5) | (1<<6);
		break;
		
		case 9: PORTC |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
		PORTD |= (1<<5) | (1<<6);
		PORTD &= ~(1<<4);
		break;
	}
}

void number (unsigned int vse_chislo)
{
	r1_1000 = vse_chislo/1000;
	r2_100 = vse_chislo%1000/100;
	r3_10 = vse_chislo%100/10;
	r4_1 = vse_chislo%10;
}


ISR (TIMER0_OVF_vect)
{
	z++;
	if (z > 4) z = 1;
	
	if (z == 1)
	{
		PORTB |= (1<<3) | (1<<4) | (1<<5);	// switch off 2, 3 and 4 bits
		PORTB &= ~(1<<2);					// switch on 1 bit
		digits (r1_1000);
	}
	else if(z == 2)
	{
		PORTB |= (1<<2) | (1<<4) | (1<<5);	// switch off 1, 3 and 4 bits
		PORTB &= ~(1<<3);					// switch on 2 bit
		digits (r2_100);
	}
	else if(z == 3)
	{
		PORTB |= (1<<2) | (1<<3) | (1<<5);	// switch off 1, 2 and 4 bits
		PORTB &= ~(1<<4);					// switch on 3 bit
		digits (r3_10);
	}
	else if(z == 4)
	{
		PORTB |= (1<<2) | (1<<3) | (1<<4); // switch off 1, 2 and 3 bits
		PORTB &= ~(1<<5);					// switch on 4 bit
		digits (r4_1);
	}
}

ISR(INT0_vect)
{
	count_pulses = count_pulses + (TCNT1 + count_interrupts*65536);
	TCNT1 = 0;
	count_interrupts = 0;
	count_10++;
	if(count_10 >= 10)
	{
		average_time = (float)count_pulses/10.0;
		count_pulses = 0;
		count_10 = 0;
	}
	
}

ISR(TIMER1_OVF_vect)
{
	count_interrupts++;
}

int main(void)
{
	//a,b,c,d
	DDRC |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
	PORTC &= ~((1<<0) | (1<<1) | (1<<2) | (1<<3));
	
	//e,f,g,dp
	DDRD |= (1<<4) | (1<<5) | (1<<6) | (1<<7);
	PORTD &= ((1<<4) | (1<<5) | (1<<6) | (1<<7));
	
	//1,2,3,4
	DDRB |= (1<<2) | (1<<3) | (1<<4) | (1<<5);
	PORTB |= (1<<2) | (1<<3) | (1<<4) | (1<<5);
	
	//INPUT
	DDRD &= ~(1<<2);
	PORTD |= (1<<2);
	
	//Counter0
	TCCR0 |= (1<<1); // clk/8
	TCNT0 = 0;
	TIMSK |= (1<<0); //Timer/Counter0 Overflow Interrupt Enable
	
	//Counter1
	TCCR1B |= (1<<0); //clk/1
	TCNT1 = 0;
	TIMSK |= (1<<2); //Timer/Counter1 Overflow Interrupt Enable
	
	//The falling edge of INT0 (PD2) generates an interrupt request
	MCUCR |= (1<<1);
	MCUCR &= ~(1<<0);
	
	GICR |= (1<<6); //INT0: External Interrupt Request 0 Enable

	sei(); //Global Interrupt Enable
	
	while (1)
	{
		number((double)8000000.0/average_time); //Frequency meter
		//number(1234); //test
		
	}
}



