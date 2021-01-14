CFLAGS = -Wall -Wextra -pedantic -g -DDEBUG

all: kbd_light

install: all
	cp kbd_light /usr/local/sbin/
	cp kbd_light.service /etc/systemd/system/
