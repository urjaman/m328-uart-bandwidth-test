/* See file COPYING. */

void uart_init(void);

#define BAUD 2000000
//#define BAUD 1000000
//#define BAUD 500000
//#define BAUD 115200
//#define BAUD 9600

#if BAUD == 115200
#define BAUD_TOL 3
#endif
