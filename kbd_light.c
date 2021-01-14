#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PATH_EV_PREFIX "/dev/input/by-path/"

char const * ev_files[] = {
	PATH_EV_PREFIX "platform-i8042-serio-0-event-kbd",
	PATH_EV_PREFIX "platform-i8042-serio-1-event-mouse",
	PATH_EV_PREFIX "platform-i8042-serio-2-event-mouse",
	PATH_EV_PREFIX "platform-thinkpad_acpi-event"
};

int main() {
	int fd = open(ev_files[0], O_RDONLY);
	
	if(fd == -1) {
		perror("[!] open");
		return 1;
	}

	while(1) {
		uint8_t buf[4096];
		ssize_t nb_chrs = read(fd, buf, 4096);

		if(nb_chrs < 0) {
			perror("[!] read");
			close(fd);
			return 1;
		}

		printf("read: ");
		for(ssize_t i = 0; i < nb_chrs; ++i)
			printf("%02x ", buf[i]);
		putchar('\n');
	}
}
