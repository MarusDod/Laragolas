prog=laragolas.out
install_path=/usr/local
bin_path=$(install_path)/bin
desktop_path=$(install_path)/share/applications
shortcut_path=/home/$(SUDO_USER)/Desktop
config_path=/etc/laragolas
icon_path=/usr/share/icons
U=$(shell whoami)

CONFIG="host: localhost\n"
CONFIG+="mysql-path: $(shell mysqld --verbose --help | grep ^datadir | awk '{print $$2}')\n"
CONFIG+="mysql-port: $(shell mysqld --verbose --help | grep ^port | awk 'NR==1 {print $$2}')\n"
CONFIG+="root-path:/var/www/\n"
CONFIG+="port: 8000"

all:$(prog)

$(prog): main.c gui.glade
	gcc -g -O0 -o $@  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic


release: main.c gui.glade
	gcc -O2 -DRELEASE -DHOME_PATH=\"$(HOME)\" -o $(prog)  main.c -Wall -Wno-unused-result `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic


.phony: clean install uninstall release

clean:
	rm -f *.out
	rm -f *~
	rm -f laragolas.desktop

echo:
	echo "\n\n\n\n"
	echo $(CONFIG)


install:
	cp ./laragolas.out $(bin_path)/laragolas.out
	cp ./laragolas.desktop $(desktop_path)/laragolas.desktop
	cp ./laragolas.png $(icon_path)/laragolas.png
	mkdir -p $(config_path)
	echo $(CONFIG) > $(config_path)/config.txt
	cp gui.glade icon.jpeg add_entry.sh logo.jpeg $(config_path)/
	cp laragolas.desktop $(shortcut_path)/laragolas.desktop
	chmod +x $(shortcut_path)/laragolas.desktop

uninstall:
	rm -f $(bin_path)/laragolas.out
	rm -f $(desktop_path)/laragolas.desktop
	rm -f $(icon_path)/laragolas.png
	rm -rf $(config_path)
	rm -f $(shortcut_path)/laragolas.desktop


