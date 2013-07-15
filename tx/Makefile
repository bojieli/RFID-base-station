all:
	gcc -Wall -O2 -g -o main main.c nrf.c -lwiringPi -lpthread -lm
	gcc -Wall -O2 -g -o simple main.c nrf_simple.c -lwiringPi -lpthread -lm

clean:
	@rm -f main simple
