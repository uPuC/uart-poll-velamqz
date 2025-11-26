#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <avr/io.h>

// ===== API p�blica =====
void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop);
void UART_putchar(uint8_t com, char data);
void UART_puts(uint8_t com, const char *str);
uint8_t UART_available(uint8_t com);
char UART_getchar(uint8_t com);

// Utilidades
void UART_gets(uint8_t com, char *buf, uint16_t maxlen, uint8_t echo);
uint16_t atoi_u16(const char *s);
void itoa_u16(uint16_t v, char *buf);

// ANSI helpers
void UART_clrscr(uint8_t com);
void UART_gotoxy(uint8_t com, uint8_t row, uint8_t col);
void UART_setColor(uint8_t com, uint8_t color);

// Colores ANSI (b�sicos)
enum {
  ANSI_RESET  = 0,
  ANSI_RED    = 31,
  ANSI_GREEN  = 32,
  ANSI_YELLOW = 33,
  ANSI_BLUE   = 34,
  ANSI_MAGENTA= 35,
  ANSI_CYAN   = 36,
  ANSI_WHITE  = 37
};

#endif // UART_H
