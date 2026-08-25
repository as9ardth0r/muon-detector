#ifndef UART_REPORT_H
#define UART_REPORT_H

#include <stddef.h>

void uart_report_init(unsigned int baudrate);
void uart_report_send(const char *text, size_t len);

#endif /* UART_REPORT_H */
