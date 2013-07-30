COMMON_DIR=../common
INCLUDE_DIR=${COMMON_DIR}
BUILD_DIR=../build
CFLAGS=-Wall -g -O2 -I${INCLUDE_DIR}

all:
	gcc ${CFLAGS} -o ${BUILD_DIR}/test-receiver test.c nrf.c sms.c ${COMMON_DIR}/common.c ${COMMON_DIR}/config.c ${COMMON_DIR}/http.c -lwiringPi
	gcc ${CFLAGS} -o ${BUILD_DIR}/receiver receiver.c sender.c nrf.c ${COMMON_DIR}/common.c ${COMMON_DIR}/config.c ${COMMON_DIR}/http.c -lwiringPi

clean:
	@rm -f ${BUILD_DIR}/test-receiver ${BUILD_DIR}/receiver
