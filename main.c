/* See file COPYING. */
#include "main.h"
#include "uart.h"

void main(void) {
	uint8_t tok = 0;
	uint8_t cnt = 0;
	uint8_t get_send(void) {
		uint8_t r = (tok << 4) | cnt;
		cnt = 0;
		tok++;
		if (tok>=13) tok = 0;
		return r;
	}

	uint8_t send = 0;
	uint8_t lsend = 0;
	uint8_t lsbyte1 = 0;
	uint8_t lsbyte2 = 0;
	uart_init();
	for(;;) {
		uint8_t stat = UCSR0A;
		if (stat & _BV(RXC0)) {
			uint8_t x = UDR0;
			if ((!lsend)&&(x & 0xFE)) {
				lsend = 2;
				lsbyte1 = 0xFF;
				lsbyte2 = x;
			} else {
				send = x&0x01;
				cnt++;
				if (cnt==15) {
					lsbyte1 = get_send();
					lsend = 1;
				}
			}
		}
		if (stat & _BV(UDRE0)) {
			if (lsend) {
				UDR0 = lsbyte1;
				lsbyte1 = lsbyte2;
				lsend--;
			} else if (send) {
				UDR0 = get_send();
			}
		}
	}
}
