prog=laragolas.out
install_path=~/.local
U=$(shell whoami)

all:$(prog)

$(prog): main.c gui.glade
	gcc -g -O0 -o $@  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic


release: main.c gui.glade
	gcc -g -DRELEASE -DHOME_PATH=\"$(HOME)\" -O0 -o $(prog)  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic


.phony: clean install uninstall release

clean:
	rm $(prog)

install:
	sed "s/user/$U/g" laragolas.desktop.in > laragolas.desktop
	cp ./laragolas.out $(install_path)/bin/laragolas.out
	cp ./laragolas.desktop $(install_path)/share/applications/laragolas.desktop
	cp ./laragolas.png $(install_path)/share/icons/laragolas.png
	mkdir -p ~/.config/laragolas
	cp gui.glade icon.jpeg config.txt logo.jpeg ~/.config/laragolas/

uninstall:
	rm $(install_path)/bin/laragolas.out
	rm $(install_path)/share/applications/laragolas.desktop
	rm $(install_path)/share/icons/laragolas.png
	rm -rf ~/.config/laragolas

