#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif
#include <cr_section_macros.h>

#include <stdio.h>

#define PCONP (*(volatile int *)0x400FC0C4)
#define PCLKSEL0 (*(volatile int *)0x400FC1A8)

#define U0THR (*(volatile int *)0x4000C000)
#define U0DLL (*(volatile int *)0x4000C000)
#define U0DLM (*(volatile int *)0x4000C004)
#define U0FCR (*(volatile int *)0x4000C008)
#define U0LCR (*(volatile int *)0x4000C00C)
#define U0LSR (*(volatile int *)0x4000C014)
#define U0FDR (*(volatile int *)0x4000C028)

#define EEPROM_ADDRESS_WRITE 0xA0
#define EEPROM_ADDRESS_READ 0xA1

//#define I2C0CONSET (*(volatile int *)0x4001C000)
//#define I2C0STAT (*(volatile int *)0x4001C004)
//#define I2C0ADR0 (*(volatile int *)0x4001C00C)
//#define I2C0SCLH (*(volatile int *)0x4001C010)
//#define I2C0SCLL (*(volatile int *)0x4001C014)
//#define I2C0DAT (*(volatile int *)0x4001C008)
//#define I2C0CONCLR (*(volatile int *)0x4001C018)

#define I2C0CONSET (*(volatile int *)0x4005C000)
#define I2C0STAT (*(volatile int *)0x4005C004)
#define I2C0ADR0 (*(volatile int *)0x4005C00C)
#define I2C0SCLH (*(volatile int *)0x4005C010)
#define I2C0SCLL (*(volatile int *)0x4005C014)
#define I2C0DAT (*(volatile int *)0x4005C008)
#define I2C0CONCLR (*(volatile int *)0x4005C018)

#define TEMP_ADDRESS (0b1001000 << 1)

#define CONSET_AA (1 << 2)
#define CONSET_SI (1 << 3)
#define CONSET_STO (1 << 4)
#define CONSET_STA (1 << 5)
#define CONSET_I2EN (1 << 6)

#define CONCLR_AAC (1 << 2)
#define CONCLR_SIC (1 << 3)
#define CONCLR_STAC (1 << 5)
#define CONCLR_I2ENC (1 << 6)

#define PINSEL0 (*(volatile int *)0x4002C000)
#define PINSEL1 (*(volatile int *)0x4002C004)
#define PINSEL3 (*(volatile int *)0x4002C00C)
#define PINSEL4 (*(volatile int *)0x4002C010)

#define PINMODE0 (*(volatile int *) 0x4002C040)
#define PINMODE1 (*(volatile int *) 0x4002C044)

#define T2IR (*(volatile int *)0x40090000)
#define T2TCR (*(volatile int * )0x40090004)
#define T2TC (*(volatile int * )0x40090008)
#define T2PR (*(volatile int * )0x4009000C)
#define T2PC (*(volatile int * )0x40090010)
#define T2MCR (*(volatile int * )0x40090014)
#define T2MR0 (*(volatile int * )0x40090018)
#define T2MR1 (*(volatile int * )0x4009001C)
#define T2EMR (*(volatile int *)0x4009003C)
#define T2CTCR (*(volatile int *)0x40090070)

void Start0() {
  I2C0CONSET = CONCLR_SIC;
  I2C0CONSET = CONSET_STA;
  I2C0CONCLR = CONCLR_SIC;
  while (((I2C0CONSET >> 3) & 1) == 0) {
  }
  I2C0CONCLR = CONCLR_STAC;
}
void Stop0() {
  I2C0CONSET = CONSET_STO;
  I2C0CONCLR = CONCLR_SIC;
  while (((I2C0CONSET >> 4) & 1) == 1) {
  }
}

int Write0(int data) {
  I2C0DAT = data;
  I2C0CONCLR = CONCLR_SIC;
  while (((I2C0CONSET >> 3) & 1) == 0) {
  }
  return I2C0STAT;
}

int Read0(int ack) {
  if (ack)
    I2C0CONSET = CONSET_AA;
  else
    I2C0CONCLR = CONCLR_AAC;
  I2C0CONCLR = CONCLR_SIC;
  while (((I2C0CONSET >> 3) & 1) == 0) {
  }
  return I2C0DAT & 255;
}

void delay_ms(float ms) {
	int sec_count = ms * 5e2;
	while (sec_count > 0){
		sec_count--;
	}
}

float Temp_Read_Cel(void) {
  int MSB;
  int LSB;
  Start0();
  Write0(TEMP_ADDRESS | 0);
  Write0(0x00);
  Start0();
  Write0(TEMP_ADDRESS | 1);
  MSB = Read0(1);
  LSB = Read0(0);
  Stop0();
  return 0.5 * ((MSB << 1) | (LSB >> 7));
}

void U0SendChar(char c) {
  while (!(U0LSR & (1 << 5)));

  U0THR = (int)c & 0xFF;
}

void U0Write(const char *s) {
  while (*s)
    U0SendChar(*s++);
}

void mem_write_byte(int index, int data){
	int address = index;
	Start0();
	Write0(EEPROM_ADDRESS_WRITE);
	Write0(address);
	Write0(data);
	Stop0();
}

int mem_read(int index){
	int address = index & 0xFF;
	int data;
	Start0();
	Write0(EEPROM_ADDRESS_WRITE);
	Write0(address);

	Start0();
	Write0(EEPROM_ADDRESS_READ);
	data = Read0(0);
	Stop0();
	return data & 0xFF;
}

void set_freq(int freq_hz) {

	T2TCR = 0x02;

	T2PR = 0;

	int period = 1000000 / freq_hz;

	T2MR1 = (period/2) - 1;

	T2MCR = 1 << 4;

	T2EMR |= 3 << 6;

	T2TCR = 0x01;
}

void initialization() {
  // UART Block
  PCONP |= 1 << 3;
  PINSEL0 &= ~(1 << 5);
  PINSEL0 |= 1 << 4;

  PINSEL0 &= ~(1 << 7);
  PINSEL0 |= 1 << 6;

  PINMODE0 |= (1 << 5);
  PINMODE0 &= ~(1 << 4);

  PINMODE0 |= (1 << 7);
  PINMODE0 &= ~(1 << 6);

  // PCLKSEL0 &= ~(1 << 7);
  // PCLKSEL0 |= (1 << 6);

  U0LCR = 0x83;
  U0DLM = 0;
  U0DLL = 12;
  U0FDR = (13 << 4) | 1;
  U0LCR = 0x03;
  U0FCR = 0x07;
  //Temp I2C1
  PINSEL0 |= (1 << 0);
      PINSEL0 |= (1 << 1);
      PINSEL0 |= (1 << 2);
      PINSEL0 |= (1 << 3);

  PINMODE0 |= (1 << 1);
  PINMODE0 &= ~(1 << 0);
  PINMODE0 |= (1 << 3);
  PINMODE0 &= ~(1 << 2);


  // I2C Block
  PCONP |= 1 << 7;

  PINSEL1 &= ~(1 << 23);
  PINSEL1 |= 1 << 22;
  PINSEL1 &= ~(1 << 25);
  PINSEL1 |= 1 << 24;

  PINMODE1 |= (1 << 23);
  PINMODE1 &= ~(1 << 22);
  PINMODE1 |= (1 << 25);
  PINMODE1 &= ~(1 << 24);

  I2C0SCLH = 5;
  I2C0SCLL = 5;

  I2C0CONSET = CONSET_SI;
  I2C0CONCLR = CONCLR_STAC;
  I2C0CONCLR = CONCLR_I2ENC;
  I2C0CONSET = CONSET_I2EN;

  // MAT for PWM block
  PCONP |= 1 << 22;

  PINSEL0 |= 1 << 15;
  PINSEL0 |= 1 << 14;

  PINMODE0 |= (1 << 15);
  PINMODE0 &= ~(1 << 14);

}

int main(void) {
  initialization();

  set_freq(1000);
  delay_ms(1000);
  while (1) {
	  set_freq(400);
	  delay_ms(1000);
	  T2TCR = 0;

	  set_freq(500);
	  delay_ms(1000);
	  T2TCR = 0;

	  set_freq(1000);
	  delay_ms(1000);
	  T2TCR = 0;
  }
}
