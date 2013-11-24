COMMON_DIR=../common
INCLUDE_DIR=${COMMON_DIR}
BUILD_DIR=../build
CFLAGS=-Wall -g -O2 -I${INCLUDE_DIR}

default: test-receiver receiver

test-receiver:
	gcc ${CFLAGS} -o ${BUILD_DIR}/test-receiver test.c nrf.c sms.c ${COMMON_DIR}/common.c ${COMMON_DIR}/config.c ${COMMON_DIR}/http.c -lwiringPi -lpthread

receiver:
	gcc ${CFLAGS} -o ${BUILD_DIR}/receiver receiver.c sender.c nrf.c ${COMMON_DIR}/common.c ${COMMON_DIR}/config.c ${COMMON_DIR}/http.c -lwiringPi -lpthread

fake-receiver:
	gcc ${CFLAGS} -o ${BUILD_DIR}/fake-receiver fake-receiver.c sender.c ${COMMON_DIR}/common.c ${COMMON_DIR}/config.c ${COMMON_DIR}/http.c -lpthread

search-channel:
	gcc ${CFLAGS} -o ${BUILD_DIR}/search-channel search-channel.c nrf.c ${COMMON_DIR}/common.c ${COMMON_DIR}/config.c ${COMMON_DIR}/http.c -lwiringPi -lpthread

clean:
	@rm -f ${BUILD_DIR}/test-receiver ${BUILD_DIR}/receiver
