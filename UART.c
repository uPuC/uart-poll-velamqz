#include "UART.h"
#include <avr/io.h>

#ifndef F_CPU
# define F_CPU 16000000UL
#endif

// ===== Validacion de puerto =====
#define VALIDATE_COM(com) if (com >= 4) return
#define VALIDATE_COM_RET(com, ret) if (com >= 4) return ret

// ===== Arrays de registros para acceso directo =====
static volatile uint8_t* const UCSRA_REGS[] = {&UCSR0A, &UCSR1A, &UCSR2A, &UCSR3A};
static volatile uint8_t* const UCSRB_REGS[] = {&UCSR0B, &UCSR1B, &UCSR2B, &UCSR3B};
static volatile uint8_t* const UCSRC_REGS[] = {&UCSR0C, &UCSR1C, &UCSR2C, &UCSR3C};
static volatile uint8_t* const UBRRH_REGS[] = {&UBRR0H, &UBRR1H, &UBRR2H, &UBRR3H};
static volatile uint8_t* const UBRRL_REGS[] = {&UBRR0L, &UBRR1L, &UBRR2L, &UBRR3L};
static volatile uint8_t* const UDR_REGS[]   = {&UDR0, &UDR1, &UDR2, &UDR3};

// ===== Helper para calculo de error =====
static uint32_t calc_error(uint32_t real, uint32_t target) {
    uint32_t diff = (real > target) ? (real - target) : (target - real);
    return (diff * 10000UL) / target;
}

// ===== Calculo de baudios con/sin U2X, eligiendo el de menor error =====
static void uart_set_baud(uint8_t com, uint32_t baud) {
    if (baud == 0) baud = 9600;

    // modo normal (U2X=0) y doble velocidad (U2X=1)
    uint32_t ubrr_norm = (F_CPU + (baud * 8UL)) / (16UL * baud) - 1UL;
    uint32_t ubrr_u2x  = (F_CPU + (baud * 4UL)) / (8UL * baud) - 1UL;

    // Calcular baudrates reales
    uint32_t baud_norm = F_CPU / (16UL * (ubrr_norm + 1UL));
    uint32_t baud_u2x  = F_CPU / (8UL * (ubrr_u2x + 1UL));

    // Calcular errores
    uint32_t err_norm = calc_error(baud_norm, baud);
    uint32_t err_u2x  = calc_error(baud_u2x, baud);

    volatile uint8_t *UCSRnA = UCSRA_REGS[com];
    volatile uint8_t *UBRRnH = UBRRH_REGS[com];
    volatile uint8_t *UBRRnL = UBRRL_REGS[com];

    if (err_u2x <= err_norm) {
        *UCSRnA |= (1 << U2X0);
        *UBRRnH = (ubrr_u2x >> 8) & 0x0F;
        *UBRRnL = ubrr_u2x & 0xFF;
    } else {
        *UCSRnA &= ~(1 << U2X0);
        *UBRRnH = (ubrr_norm >> 8) & 0x0F;
        *UBRRnL = ubrr_norm & 0xFF;
    }
}

// ===== Inicializacion =====
void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop) {
    VALIDATE_COM(com);

    uart_set_baud(com, baudrate);

    volatile uint8_t *UCSRnB = UCSRB_REGS[com];
    volatile uint8_t *UCSRnC = UCSRC_REGS[com];

    // Tamano de palabra (5..8 bits)
    if (size < 5) size = 5;
    if (size > 8) size = 8;
    uint8_t UCSZ = size - 5;  // 5->0, 6->1, 7->2, 8->3

    // Paridad: 0=none, 1=odd (11), 2=even (10)
    uint8_t UPM = (parity == 1) ? 0b11 : (parity == 2 ? 0b10 : 0b00);

    // Stop bits: 1 o 2
    uint8_t USBS = (stop == 2) ? 1 : 0;

    // Configurar modo asincrono
    *UCSRnC = (UPM << 4) | (USBS << 3) | (UCSZ << 1);

    // Habilitar RX/TX
    *UCSRnB = (1 << RXEN0) | (1 << TXEN0);
}

// ===== TX por sondeo =====
void UART_putchar(uint8_t com, char data) {
    VALIDATE_COM(com);
    volatile uint8_t *UCSRnA = UCSRA_REGS[com];
    volatile uint8_t *UDRn = UDR_REGS[com];

    // Espera buffer vacio
    while (!(*UCSRnA & (1 << UDRE0)));

    *UDRn = data;
}

void UART_puts(uint8_t com, const char *str) {
    if (!str) return;
    while (*str) {
        UART_putchar(com, *str++);
    }
}

// ===== RX por sondeo =====
uint8_t UART_available(uint8_t com) {
    VALIDATE_COM_RET(com, 0);
    volatile uint8_t *UCSRnA = UCSRA_REGS[com];
    return (*UCSRnA & (1 << RXC0)) ? 1 : 0;
}

char UART_getchar(uint8_t com) {
    VALIDATE_COM_RET(com, 0);
    volatile uint8_t *UCSRnA = UCSRA_REGS[com];
    volatile uint8_t *UDRn = UDR_REGS[com];

    // Espera byte recibido
    while (!(*UCSRnA & (1 << RXC0)));

    return *UDRn;
}

// ===== Utilidades =====
void UART_gets(uint8_t com, char *buf, uint16_t maxlen, uint8_t echo) {
    if (!buf || maxlen == 0) return;

    uint16_t i = 0;
    while (i < maxlen - 1) {
        char c = UART_getchar(com);

        if (c == '\r' || c == '\n') {
            if (echo) {
                UART_putchar(com, '\r');
                UART_putchar(com, '\n');
            }
            break;
        } else if (c == 0x7F || c == 0x08) {  // Backspace/DEL
            if (i > 0) {
                i--;
                if (echo) UART_puts(com, "\b \b");
            }
        } else {
            buf[i++] = c;
            if (echo) UART_putchar(com, c);
        }
    }
    buf[i] = '\0';
}

uint16_t atoi_u16(const char *s) {
    uint32_t v = 0;
    if (!s) return 0;
    
    while (*s >= '0' && *s <= '9') {
        v = v * 10u + (*s - '0');
        if (v > 65535u) return 65535u;
        s++;
    }
    return v;
}

void itoa_u16(uint16_t v, char *buf) {
    char tmp[6];
    int i = 0;
    
    if (v == 0) {
        tmp[i++] = '0';
    } else {
        while (v > 0) {
            tmp[i++] = '0' + (v % 10u);
            v /= 10u;
        }
    }
    
    // Invertir a buf
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

// ===== ANSI helpers =====
static void uart_putnum(uint8_t com, uint8_t n) {
    if (n >= 100) UART_putchar(com, '0' + n / 100);
    if (n >= 10) UART_putchar(com, '0' + (n / 10) % 10);
    UART_putchar(com, '0' + n % 10);
}

void UART_clrscr(uint8_t com) {
    UART_puts(com, "\x1B[2J\x1B[H");
}

void UART_gotoxy(uint8_t com, uint8_t row, uint8_t col) {
    UART_putchar(com, '\x1B');
    UART_putchar(com, '[');
    uart_putnum(com, row);
    UART_putchar(com, ';');
    uart_putnum(com, col);
    UART_putchar(com, 'H');
}

void UART_setColor(uint8_t com, uint8_t color) {
    UART_putchar(com, '\x1B');
    UART_putchar(com, '[');
    
    if (color == 0) {
        UART_putchar(com, '0');
    } else {
        uart_putnum(com, color);
    }
    
    UART_putchar(com, 'm');
}
