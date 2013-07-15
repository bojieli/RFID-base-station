CFLAGS=-Wall -g -O2
REMOTE_HOST=pi
REMOTE_DIR=~/rfid-base-station

default:
	gcc ${CFLAGS} -o merger merger.c server.c sender.c timeout.c dict.c http.c config.c -lpthread

upload:
	scp -r . ${REMOTE_HOST}:${REMOTE_DIR}/merger/
	ssh ${REMOTE_HOST} 'cd ${REMOTE_DIR}/merger; make testclient'

testclient: testclient.c
	gcc ${CFLAGS} -o testclient testclient.c

test: testclient
	ssh ${REMOTE_HOST} 'cd ${REMOTE_DIR}/merger; ./testclient' >/dev/null 2>&1 &
	./testclient

clean:
	@rm -f merger testclient merger.log
