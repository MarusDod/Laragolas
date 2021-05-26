all:gui.out

gui.out: main.c gui.glade
	gcc -g -O0 -o $@  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic

clean:
	rm gui.out
