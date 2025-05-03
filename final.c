#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif
#include <cr_section_macros.h>

#include <stdio.h>
#include <stdlib.h>

#define PCONP (*(volatile int *)0x400FC0C4)
#define PCLKSEL0 (*(volatile int *)0x400FC1A8)

#define U0THR (*(volatile int *)0x4000C000)
#define U0RBR (*(volatile int *)0x4000C000)
#define U0DLL (*(volatile int *)0x4000C000)
#define U0DLM (*(volatile int *)0x4000C004)
#define U0FCR (*(volatile int *)0x4000C008)
#define U0LCR (*(volatile int *)0x4000C00C)
#define U0LSR (*(volatile int *)0x4000C014)
#define U0FDR (*(volatile int *)0x4000C028)

#define EEPROM_ADDRESS_WRITE 0xA0
#define EEPROM_ADDRESS_READ 0xA1

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
#define PINSEL3 (*(volatile int *)0x4002C00C)
#define PINSEL4 (*(volatile int *)0x4002C010)

#define PINMODE0 (*(volatile int *)0x4002C040)
#define PINMODE1 (*(volatile int *)0x4002C044)

#define T2IR (*(volatile int *)0x40090000)
#define T2TCR (*(volatile int *)0x40090004)
#define T2TC (*(volatile int *)0x40090008)
#define T2PR (*(volatile int *)0x4009000C)
#define T2PC (*(volatile int *)0x40090010)
#define T2MCR (*(volatile int *)0x40090014)
#define T2MR0 (*(volatile int *)0x40090018)
#define T2MR1 (*(volatile int *)0x4009001C)
#define T2EMR (*(volatile int *)0x4009003C)
#define T2CTCR (*(volatile int *)0x40090070)

#define RTC_ILR (*(volatile int *)0x40024000)
#define RTC_CCR (*(volatile int *)0x40024008)
#define RTC_CTIME0 (*(volatile int *)0x40024014)
#define RTC_SEC (*(volatile int *)0x40024020)
#define RTC_MIN (*(volatile int *)0x40024024)
#define RTC_HOUR (*(volatile int *)0x40024028)

#define CCR_CLKEN (1 << 0)
#define CCR_CTCRST (1 << 1)

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

void U0SendChar(char c) {
    while (!(U0LSR & (1 << 5)))
        ;

    U0THR = (int)c & 0xFF;
}

void U0Write(const char *s) {
    while (*s)
        U0SendChar(*s++);
}

void mem_write_byte(int index, int data) {
    int address = index;
    Start0();
    Write0(EEPROM_ADDRESS_WRITE);
    Write0(address);
    Write0(data);
    Stop0();
    delay_ms(25);
}

int mem_read(int index) {
    int address = index & 0xFF;
    int data;
    Start0();
    Write0(EEPROM_ADDRESS_WRITE);
    Write0(address);

    Start0();
    Write0(EEPROM_ADDRESS_READ);
    data = Read0(0);
    Stop0();
    delay_ms(25);
    return data & 0xFF;
}

void set_freq(int freq_hz) {

    T2TCR = 0x02;

    T2PR = 0;

    int period = 1000000 / freq_hz;

    T2MR1 = (period / 2) - 1;

    T2MCR = 1 << 4;

    T2EMR |= 3 << 6;

    T2TCR = 0x01;
}

typedef struct {
    char pitch;
    int beats;
} note;

int note_lookup(char note) {
    switch (note) {
    case 'E':
        return 330;
    case 'g':
        return 370;
    case 'G':
        return 392;
    case 'A':
        return 440;
    case 'B':
        return 494;
    case 'd':
        return 554;
    case 'D':
        return 587;
    default:
        return 400;
    }
}

static const note halo_tune[15] = {{'E', 1}, {'g', 1}, {'G', 1}, {'g', 1}, {'A', 1}, {'G', 1}, {'g', 1}, {'E', 2},
                                   {'B', 1}, {'d', 1}, {'D', 2}, {'d', 1}, {'A', 1}, {'d', 1}, {'B', 2}};

void play_note(note note) {
    int freq = note_lookup(note.pitch);
    set_freq(freq);
    delay_ms(note.beats * 750);
    T2TCR = 0;
}

void landing_tune() {
    for (int i = 0; i < 15; i++) {
        play_note(halo_tune[i]);
    }
}

int get_sec() {
    int time = RTC_CTIME0;
    return time & 0x3F;
}
int get_min() {
    int time = RTC_CTIME0;
    return (time >> 8) & 0x3F;
}
int get_hour() {
    int time = RTC_CTIME0;
    return (time >> 16) & 0x3F;
}

int U0GetChar() {
    while (!(U0LSR & 1))
        ;
    return U0RBR & 0xFF;
}

void U0GetTime(char time[]) {
    int idx = 0;
    char c;
    while (idx < 6) {
        c = (char)U0GetChar();
        U0SendChar(c);
        time[idx] = c;
        idx++;
    }
    U0Write("\r\n");
    time[6] = '\0';
}

void concat_chars(char fin[], char first, char second) {
    fin[0] = first;
    fin[1] = second;
    fin[2] = '\0';
}

void landing_sequence() {
    char c = 'n';
    while (1) {
        U0Write("Has the eagle landed? y/n   ");
        c = U0GetChar();
        U0SendChar(c);
        U0Write("\r\n");
        if (c == 'y') {
            U0Write("Eagle has landed, initializing startup");
            break;
        }
    }
    landing_tune();
    U0Write("\r\n");
    U0Write("\r\n");
}

void collect_data(int idx) {
    int actual_index = idx * 6;

    int temp = Temp_Read_Cel();
    int t1 = get_min();
    int t2 = get_sec();

    char temp_buff[2];
    char t1_buff[2];
    char t2_buff[2];

    if (temp < 10) {
        sprintf(temp_buff, "0%d", temp);
    } else {
        sprintf(temp_buff, "%d", temp);
    }
    if (t1 < 10) {
        sprintf(t1_buff, "0%d", t1);
    } else {
        sprintf(t1_buff, "%d", t1);
    }
    if (t2 < 10) {
        sprintf(t2_buff, "0%d", t2);
    } else {
        sprintf(t2_buff, "%d", t2);
    }

    mem_write_byte(actual_index, t1_buff[0]);
    mem_write_byte(actual_index + 1, t1_buff[1]);

    mem_write_byte(actual_index + 2, t2_buff[0]);
    mem_write_byte(actual_index + 3, t2_buff[1]);

    mem_write_byte(actual_index + 4, temp_buff[0]);
    mem_write_byte(actual_index + 5, temp_buff[1]);
}

void spew_data(int idx) {
    int count = 0;
    while (count < 20) {
        int actual_idx = (idx + count) * 6;

        char temp_buff[3];
        char t1_buff[3];
        char t2_buff[3];

        t1_buff[0] = mem_read(actual_idx);
        t1_buff[1] = mem_read(actual_idx + 1);
        t1_buff[2] = '\0';

        t2_buff[0] = mem_read(actual_idx + 2);
        t2_buff[1] = mem_read(actual_idx + 3);
        t2_buff[2] = '\0';

        temp_buff[0] = mem_read(actual_idx + 4);
        temp_buff[1] = mem_read(actual_idx + 5);
        temp_buff[2] = '\0';

        U0Write("Time ");
        U0Write(t1_buff);
        U0Write(":");
        U0Write(t2_buff);
        U0Write(", ");
        U0Write("Temp ");
        U0Write(temp_buff);
        U0Write("\r\n");
        count++;
    }
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
    // Temp I2C1
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

void initialize_time() {
    // RTC Block
    PCONP |= 1 << 9;

    char buff[7];

    U0Write("Please enter time, with a leading zero if needed.\r\n");
    U0Write("Write it in this format:hhmmss\r\n");
    U0GetTime(buff);

    char sec[3];
    char min[3];
    char hr[3];
    concat_chars(sec, buff[4], buff[5]);
    concat_chars(min, buff[2], buff[3]);
    concat_chars(hr, buff[0], buff[1]);

    U0Write("Hour: ");
    U0Write(hr);
    U0Write("\r\n");

    U0Write("Minute: ");
    U0Write(min);
    U0Write("\r\n");

    U0Write("Second: ");
    U0Write(sec);
    U0Write("\r\n");

    RTC_CCR = CCR_CTCRST;
    RTC_SEC = atoi(sec);
    RTC_MIN = atoi(min);
    RTC_HOUR = atoi(hr);

    RTC_CCR = CCR_CLKEN;
}

int main(void) {
    initialization();
    landing_sequence();
    initialize_time();

    int idx = 0;
    int warmup = 0;
    while (1) {
        if (get_sec() % 3 == 0) {
            collect_data(idx);
            idx = (idx + 1) % 20;
            delay_ms(1000);
        }
        if (idx == 19 && !warmup) {
            warmup = 1;
            delay_ms(1000);
        }
        if (idx == 0 && warmup) {
            spew_data(idx);
            warmup = 0;
            U0Write("\r\n");
            U0Write("\r\n");
            delay_ms(1000);
        }
        char c = U0RBR;
        if (c == 's') {
            initialize_time();
        }
    }
}
