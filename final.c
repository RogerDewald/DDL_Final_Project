#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif
#include <cr_section_macros.h>

#define PCLK 25000000
#define UART_BAUD 115200
int baud_rate;

#define PCONP (*(volatile int *)0x400FC0C4)
#define PCLKSEL0 (*(volatile int *)0x400FC1A8)

#define U0THR (*(volatile int *)0x4000C000)
#define U0DLL (*(volatile int *)0x4000C000)
#define U0DLM (*(volatile int *)0x4000C004)
#define U0FCR (*(volatile int *)0x4000C008)
#define U0LCR (*(volatile int *)0x4000C00C)
#define U0LSR (*(volatile int *)0x4000C014)

#define I2C0CONSET (*(volatile int *)0x4001C000)
#define I2C0STAT (*(volatile int *)0x4001C004)
#define I2C0ADR0 (*(volatile int *)0x4001C00C)
#define I2C0SCLH (*(volatile int *)0x4001C010)
#define I2C0SCLL (*(volatile int *)0x4001C014)
#define I2C0DAT (*(volatile int *)0x4001C008)
#define I2C0CONCLR (*(volatile int *)0x4001C018)

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
#define PINSEL4 (*(volatile int *)0x4002C010)

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
  volatile int sec_count = ms * 5e2;
  while (sec_count > 0) {
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

void uart0_putc(char c) {
  while (!(U0LSR & (1u << 5)))
    ;
  U0THR = (int)c;
}

void uart0_write(const char *s) {
  while (*s)
    uart0_putc(*s++);
}

int _write(int fd, char *ptr, int len) {
  (void)fd;
  for (int i = 0; i < len; ++i)
    uart0_putc(ptr[i]);
  return len;
}

void initialization() {
  // UART Block
  PINSEL0 &= ~(1 << 5);
  PINSEL0 |= 1 << 4;

  PINSEL0 &= ~(1 << 7);
  PINSEL0 |= 1 << 6;

  PCLKSEL0 &= ~(1 << 7);
  PCLKSEL0 |= (1 << 6);

  baud_rate = 100000000 / (16 * UART_BAUD);
  U0LCR = 0x83;
  U0DLM = (baud_rate >> 8) & 0xFF;
  U0DLL = baud_rate & 0xFF;
  U0LCR = 0x03;
  U0FCR = 0x07;

  // PWM Block
  PCONP |= (1 << 1);

  // I2C Block
  PCONP |= (1 << 7);

  PINSEL1 &= ~(1 << 23);
  PINSEL1 |= (1 << 22);
  PINSEL1 &= ~(1 << 25);
  PINSEL1 |= (1 << 24);

  I2C0SCLH = 5;
  I2C0SCLL = 5;

  I2C0CONSET = CONSET_SI;
  I2C0CONCLR = CONCLR_STAC;
  I2C0CONCLR = CONCLR_I2ENC;
  I2C0CONSET = CONSET_I2EN;
}

int main(void) {
  initialization();

  printf("Wow");
  while (1) {
        printf("Still Works");
        delay_ms(2000);
        
  }
}
