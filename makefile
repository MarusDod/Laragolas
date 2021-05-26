prog=laragolas.out

all:$(prog)

$(prog): main.c gui.glade
	gcc -g -O0 -o $@  main.c -Wall `pkg-config --cflags  --libs gtk+-3.0` -export-dynamic

clean:
	rm $(prog)
