#CFLAGS = -Wall -Wextra -pedantic -g -DDEBUG
CFLAGS = -Wall -Wextra -pedantic

all: kbd_light

install: all
	cp kbd_light /usr/local/sbin/
	cp kbd_light.service /etc/systemd/system/
	cp kbd_light.rules /etc/udev/rules.d/25-kbd_light.rules
	systemctl daemon-reload

uninstall:
	rm /usr/local/sbin/kbd_light
	rm /etc/systemd/system/kbd_light.service
	rm /etc/udev/rules.d/25-kbd_light.rules
	systemctl daemon-reload
