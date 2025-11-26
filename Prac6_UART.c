#include <avr/io.h>
#include <avr/interrupt.h>
#include "UART.h"

// Helper para imprimir en base 2, 10 o 16 (usa letras may?sculas)
static void u16_to_base(uint16_t v, char *buf, uint8_t base) {
    const char *digits = "0123456789ABCDEF";
    char tmp[17]; // suficiente para binario de 16 bits + '\0'
    int i = 0;

    if (base != 2 && base != 10 && base != 16) base = 10;

    if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    while (v > 0) {
        tmp[i++] = digits[v % base];
        v /= base;
    }
    // invertir a buf
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

int main(void) {
    char cad[32];
    char cadUart3[32];
    char out[32];
    uint16_t num;

    // UART0 por USB hacia PC (elige uno)
    //UART_Ini(0, 12345, 8, 0, 2);   // 12345 baudios, 8N2
	UART_Ini(0, 115200, 8, 0, 1);
    

    // UART2 y UART3: v?nculo cruzado (si los usas)
    UART_Ini(2, 115200, 8, 0, 1);
    UART_Ini(3, 115200, 8, 0, 1);


    while (1) {
        // ---- Lectura por UART0 ----
        UART_gotoxy(0, 2, 2);
        UART_setColor(0, ANSI_YELLOW);
        UART_puts(0,"Introduce un numero:");

        // mover cursor a la zona de entrada
        //UART_gotoxy(0, 2, 22);
        UART_setColor(0, ANSI_GREEN);
		UART_gets(0, cad, sizeof(cad), 1);  // echo=1 -> se ver? en verde

        // Convertir a entero
        num = atoi_u16(cad);

        // Borra zona de salida
        UART_gotoxy(0, 2, 22); UART_puts(0, "                                             ");
        UART_gotoxy(0, 3, 5); UART_puts(0, "                                             ");
        UART_gotoxy(0, 4, 5); UART_puts(0, "                                             ");
        UART_gotoxy(0, 5, 5); UART_puts(0, "                                             ");

        // Decimal (mantengo tu verde actual para la representaci?n decimal)
        UART_setColor(0, ANSI_GREEN);
        itoa_u16(num, out);  // decimal con la util del UART
        UART_gotoxy(0, 3, 5);
        UART_puts(0, out);

        // Hexadecimal en AZUL
        UART_setColor(0, ANSI_BLUE);
        u16_to_base(num, out, 16);
        UART_gotoxy(0, 4, 5);
        UART_puts(0, "Hex: ");
        UART_puts(0, out);

        // Binario en AZUL
        UART_setColor(0, ANSI_BLUE);
        u16_to_base(num, out, 2);
        UART_gotoxy(0, 5, 5);
        UART_puts(0, "Bin: ");
        UART_puts(0, out);

        UART_setColor(0, ANSI_RESET);

        // ---- (Opcional) Eco entre UART3 <-> UART2 ----
        if (UART_available(3)) {
            char c = UART_getchar(3);
            UART_putchar(2, c); // reenv?a a UART2
            // tambi?n muestra por UART0 (depuraci?n)
            UART_gotoxy(0, 9, 3);
            UART_puts(0, "Echo UART3->UART2: '");
            cadUart3[0] = c; cadUart3[1] = '\0';
            UART_puts(0, cadUart3);
            UART_puts(0, "'");
        }
    }
}
