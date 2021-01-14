#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//#define DEBUG 1

#define PATH_KBD_BACKLIGHT "/sys/class/leds/tpacpi::kbd_backlight/brightness"
#define PATH_EV_PREFIX "/dev/input/by-path/"

unsigned int const brightness_level = 1;
unsigned int const read_delay_ms = 100;
unsigned int const backlight_timeout_ms = 4000;

char const * const ev_files[] = {
	PATH_EV_PREFIX "platform-i8042-serio-0-event-kbd",
	PATH_EV_PREFIX "platform-i8042-serio-1-event-mouse",
	PATH_EV_PREFIX "platform-i8042-serio-2-event-mouse",
};
size_t ev_files_size = sizeof ev_files / sizeof(char *);


void print_raw_bytes(uint8_t * buf, size_t size) {
	printf("[%ld] ", size);
	for(size_t i = 0; i < size; ++i)
		printf("%02x ", buf[i]);
	putchar('\n');
}

void print_ev(struct input_event const * ev) {
	printf("time: %3ld.%06ld type: %hu, code: %hu, value: %u\n",
			ev->time.tv_sec, 
			ev->time.tv_usec, 
			ev->type, 
			ev->code, 
			ev->value);
}

void open_ev_files(struct pollfd * fds) {
	for(size_t i = 0; i < ev_files_size; ++i) {
		fds[i].fd = open(ev_files[i], O_RDONLY);

		if(fds[i].fd == -1) {
			char buf[1024];
			sprintf(buf, "[!] opening \"%s\"", ev_files[i]);
			perror(buf);
			while(i) close(fds[--i].fd);
			exit(1);
		}
		
		fds[i].events = POLLIN;
	}
}

void close_ev_files(struct pollfd * fds) {
	for(size_t i = 0; i < ev_files_size; ++i)
		close(fds[i].fd);
}

void consume_data(struct pollfd * fd) {
	uint8_t buf[4096];
	ssize_t nb_chrs = read(fd->fd, buf, 4096);

#ifdef DEBUG
	struct input_event * evs = (void *) buf;
	ssize_t nb_evs = nb_chrs / sizeof(struct input_event);

	print_raw_bytes(buf, nb_chrs);
	for(ssize_t i = 0; i < nb_evs; ++i)
		print_ev(evs + i);
#else
	(void) nb_chrs;
#endif
}

void consume_all_data(struct pollfd fds[]) {
	for(struct pollfd * fd = fds; fd != fds + ev_files_size; ++fd) {
		if(fd->revents & POLLERR) {
			perror("[!] poll");
			close_ev_files(fds);
			exit(1);
		}

		if(fd->revents & POLLIN)
			consume_data(fd);
	}
}

int set_kbd_light(int state) {
	char buf[10];
	int fd = open(PATH_KBD_BACKLIGHT, O_WRONLY);
	if(fd == -1) {
		perror("[!] opening \"" PATH_KBD_BACKLIGHT "\"");
		return -1;
	}

	int nb_chrs = sprintf(buf, "%d\n", state);
	write(fd, buf, nb_chrs);
	if(fd == -1) {
		perror("[!] writing in \"" PATH_KBD_BACKLIGHT "\"");
		return -1;
	}

	close(fd);
	return 0;
}

void disable_light() {
	set_kbd_light(0);
}

int main() {
	int light_state = 0;
	struct pollfd fds[ev_files_size];

	open_ev_files(fds);
	atexit(disable_light);
	signal(SIGINT, exit);

	int err = 0;
	while(1) {
		int nb = poll(fds, ev_files_size, backlight_timeout_ms);
		if(nb) {
			consume_all_data(fds);
			if(!light_state)
				err = set_kbd_light(light_state = brightness_level);
		}
		else if(light_state)
			err = set_kbd_light(light_state = 0);

		// this delay allows to reduce system calls for input events
		usleep(read_delay_ms * 1000);
	}

	close_ev_files(fds);
}
