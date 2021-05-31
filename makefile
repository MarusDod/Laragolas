prog=laragolas.out
install_path=~/.local
U=$(shell whoami)

CONFIG="host: localhost\n"
CONFIG+="mysql-path: $(shell mysqld --verbose --help | grep ^datadir | awk '{print $$2}')\n"
CONFIG+="mysql-port: $(shell mysqld --verbose --help | grep ^port | awk 'NR==1 {print $$2}')\n"
CONFIG+="root-path:${HOME}/\n"
CONFIG+="port: 8000"

all:$(prog)

$(prog): main.c gui.glade
	gcc -g -O0 -o $@  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic


release: main.c gui.glade
	gcc -g -DRELEASE -DHOME_PATH=\"$(HOME)\" -O0 -o $(prog)  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic


.phony: clean install uninstall release

clean:
	rm -f *.out
	rm -f *~
	rm -f laragolas.desktop

echo:
	echo "\n\n\n\n"
	echo $(CONFIG)


install:
	sed "s/user/$U/g" laragolas.desktop.in > laragolas.desktop
	cp ./laragolas.out $(install_path)/bin/laragolas.out
	cp ./laragolas.desktop $(install_path)/share/applications/laragolas.desktop
	cp ./laragolas.png $(install_path)/share/icons/laragolas.png
	mkdir -p ~/.config/laragolas
	echo $(CONFIG) > ~/.config/laragolas/config.txt
	cp gui.glade icon.jpeg logo.jpeg ~/.config/laragolas/
	ln -f laragolas.desktop ~/Desktop/laragolas.desktop
	chmod +x ~/Desktop/laragolas.desktop

uninstall:
	rm -f $(install_path)/bin/laragolas.out
	rm -f $(install_path)/share/applications/laragolas.desktop
	rm -f $(install_path)/share/icons/laragolas.png
	rm -rf ~/.config/laragolas
	rm -f ~/Desktop/laragolas.desktop


