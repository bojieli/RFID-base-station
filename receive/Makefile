BUILD_DIR=../build

all:
	gcc -Wall -O2 -g -o ${BUILD_DIR}/test-receiver test.c nrf.c sms.c -lwiringPi -lpthread
	gcc -Wall -O2 -g -o ${BUILD_DIR}/receiver receiver.c nrf.c -lwiringPi -lpthread

clean:
	@rm -f ${BUILD_DIR}/test-receiver ${BUILD_DIR}/receiver
