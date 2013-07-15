all:
	gcc -Wall -O2 -g -o test test.c nrf.c sms.c -lwiringPi -lpthread
	gcc -Wall -O2 -g -o receiver receiver.c nrf.c -lwiringPi -lpthread

clean:
	@rm -f test receiver receiver.log test.log
