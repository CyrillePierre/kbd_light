#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PATH_EV_PREFIX "/dev/input/by-path/"

char const * ev_files[] = {
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
	struct input_event * evs = (void *) buf;
	ssize_t nb_chrs = read(fd->fd, buf, 4096);
	ssize_t nb_evs = nb_chrs / sizeof(struct input_event);

	print_raw_bytes(buf, nb_chrs);
	for(ssize_t i = 0; i < nb_evs; ++i)
		print_ev(evs + i);
}

int main() {
	struct pollfd fds[ev_files_size];
	open_ev_files(fds);

	int cnt = 0;
	while(1) {
		poll(fds, ev_files_size, -1);
		for(struct pollfd * fd = fds; fd != fds + ev_files_size; ++fd) {
			if(fd->revents & POLLERR) {
				perror("[!] poll");
				close_ev_files(fds);
				return 1;
			}

			if(fd->revents & POLLIN) {
				consume_data(fd);
				printf("event %d\n", cnt++);
			}
		}
	}

	close_ev_files(fds);
}
