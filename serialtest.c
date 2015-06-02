/* See file COPYING. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <inttypes.h>

struct baudentry {
	int flag;
	unsigned int baud;
};
#define BAUDENTRY(baud) { B##baud, baud },

static const struct baudentry sp_baudtable[] = {
	BAUDENTRY(9600) /* unconditional default */
#if defined(B19200)
	BAUDENTRY(19200)
#endif
#if defined(B38400)
	BAUDENTRY(38400)
#endif
#if defined(B57600)
	BAUDENTRY(57600)
#endif
#if defined(B115200)
	BAUDENTRY(115200)
#endif
#if defined(B230400)
	BAUDENTRY(230400)
#endif
#if defined(B460800)
	BAUDENTRY(460800)
#endif
#if defined(B500000)
	BAUDENTRY(500000)
#endif
#if defined(B576000)
	BAUDENTRY(576000)
#endif
#if defined(B921600)
	BAUDENTRY(921600)
#endif
#if defined(B1000000)
	BAUDENTRY(1000000)
#endif
#if defined(B1152000)
	BAUDENTRY(1152000)
#endif
#if defined(B1500000)
	BAUDENTRY(1500000)
#endif
#if defined(B2000000)
	BAUDENTRY(2000000)
#endif
#if defined(B2500000)
	BAUDENTRY(2500000)
#endif
#if defined(B3000000)
	BAUDENTRY(3000000)
#endif
#if defined(B3500000)
	BAUDENTRY(3500000)
#endif
#if defined(B4000000)
	BAUDENTRY(4000000)
#endif
	{0, 0}			/* Terminator */
};

void set_baudrate(int fd, unsigned int baud) {
	int i;
	struct termios options;
	int bro = -1;
	for (i=0;sp_baudtable[i].baud;i++) {
		if (sp_baudtable[i].baud == baud) {
			bro = i;
			break;
		}
	}
	if (bro==-1) {
		printf("Cannot set baud rate %d\n",baud);
		exit(10);
	}
	tcgetattr(fd,&options);
	cfsetispeed(&options, sp_baudtable[bro].flag);
	cfsetospeed(&options, sp_baudtable[bro].flag);
	tcsetattr(fd, TCSANOW, &options);
}

int open_devfd(char * fn) {
	struct termios options;
	int fd;
	fd = open(fn,O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd == -1) {
		printf("Serial open fail\n");
		exit(2);
	}
	tcgetattr(fd,&options);
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	options.c_oflag &= ~OPOST;
	options.c_cc[VTIME] = 0;

	tcsetattr(fd, TCSANOW, &options);
	return fd;
}

static int32_t written = 0;
static int32_t readed = 0;
static int32_t readskips = 0;
static int32_t readerrs = 0;
static int32_t received = 0;
static int next_err = 0;
static int8_t next_tok = -1;

void readwrite(int fd, int wrcnt, uint8_t wrb)
{
	uint8_t wrbuf[1024];
	uint8_t rdbuf[1024];
	ssize_t r,w;
	memset(wrbuf,wrb,1024);
	do {
		r = read(fd,rdbuf,1024);
		if (r < 0) r = 0;
		for (ssize_t i=0;i<r;i++) {
			if (next_err) {
				printf("E:%02X ", rdbuf[i]);
				fflush(stdout);
				next_err = 0;
			}
			if (rdbuf[i] == 0xFF) {
				readerrs++;
				next_err = 1;
				continue;
			}
			uint8_t token = rdbuf[i] >> 4;
			received += rdbuf[i]&0xF;
			if ((next_tok >= 0)&&(token != next_tok)) {
				readskips++;
			}
			next_tok = token+1;
			if (next_tok == 13) next_tok = 0;
		}
		readed += r;
		if (wrcnt) {
			int wrdo = wrcnt>1024?1024:wrcnt;
			w = write(fd,wrbuf,wrdo);
			if (w < 0) w = 0;
			written += w;
			wrcnt -= w;
		}
	} while (r && wrcnt);
}


int main(int argc, char * argv[]) {
	int devfd;
	int baud;
	int mode;
	int size;
	if (argc < 5) {
		printf("Usage: %s serial_device baud mode seconds\n",argv[0]);
		exit(1);
	}
	baud = atoi(argv[2]);
	mode = atoi(argv[3]);
	if (baud <=  0) {
		printf("Bad baud\n");
		return 1;
	}
 	size = atoi(argv[4]) * (baud/10);
	devfd = open_devfd(argv[1]);
	set_baudrate(devfd, baud);
	printf("Connected.\n");
	sleep(3); /* Wait for the arduino bootloader to exit. */
	readwrite(devfd,1,0);
	tcdrain(devfd);
	tcflush(devfd, TCIFLUSH);
	const char * modestr;
	printf("Testing.. ");
	fflush(stdout);
	switch (mode) {
		default:
			printf("Unknown mode\n");
			return 1;
		case 0:
			modestr = "RX&TX";
			while (written < size) readwrite(devfd,size-written,1);
			readwrite(devfd,1,0);
			tcdrain(devfd);
			readwrite(devfd,0,0);
			readwrite(devfd,0,0);
			break;
		case 1:
			modestr = "RX";
			readwrite(devfd,1,1);
			while (readed < size) readwrite(devfd,0,1);
			readwrite(devfd,1,0);
			break;
		case 2:
			modestr = "TX";
			size = (size/15) * 15;
			while (written < size) readwrite(devfd,size-written, 0);
			tcdrain(devfd);
			break;
	}
	printf(" Flushing..\n");
	sleep(1); // flush...
	readwrite(devfd,1,0);
	tcdrain(devfd);
	sleep(1); // flush...
	readwrite(devfd,0,0);
	sleep(1); // flush...
	readwrite(devfd,0,0);
	printf("Done - Mode: %s(%d) dev-received / written %d / %d - errs %d, read %d readskips %d\n",
		modestr, mode, received, written, readerrs, readed, readskips);
	close(devfd);
	return 0;
}
