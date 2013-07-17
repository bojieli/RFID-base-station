BUILD_DIR=../build

all:
	gcc -Wall -O2 -g -o ${BUILD_DIR}/send main.c nrf.c -lwiringPi -lpthread -lm
	gcc -Wall -O2 -g -o ${BUILD_DIR}/simple-send main.c nrf_simple.c -lwiringPi -lpthread -lm

clean:
	@rm -f ${BUILD_DIR}/send ${BUILD_DIR}/simple-send
