COMMON_DIR=../common
INCLUDE_DIR=${COMMON_DIR}
BUILD_DIR=../build
CFLAGS=-Wall -g -O2 -I${INCLUDE_DIR}

REMOTE_HOST=pi
REMOTE_DIR=~/rfid-base-station

default:
	gcc ${CFLAGS} -o ${BUILD_DIR}/merger merger.c server.c sender.c timeout.c watchdog.c ${COMMON_DIR}/common.c ${COMMON_DIR}/dict.c ${COMMON_DIR}/http.c ${COMMON_DIR}/config.c -lpthread

test-notify:
	gcc ${CFLAGS} -o ${BUILD_DIR}/test-notify test-notify.c sender.c ${COMMON_DIR}/common.c ${COMMON_DIR}/dict.c ${COMMON_DIR}/http.c ${COMMON_DIR}/config.c -lpthread

upload:
	scp -r . ${REMOTE_HOST}:${REMOTE_DIR}/merger/
	ssh ${REMOTE_HOST} 'cd ${REMOTE_DIR}/merger; make testclient'

testclient: testclient.c
	gcc ${CFLAGS} -o ${BUILD_DIR}/testclient testclient.c

test: testclient
	ssh ${REMOTE_HOST} 'cd ${REMOTE_DIR}/${BUILD_DIR}; ./testclient' >/dev/null 2>&1 &
	${BUILD_DIR}/testclient

clean:
	@rm -f ${BUILD_DIR}/merger ${BUILD_DIR}/testclient
