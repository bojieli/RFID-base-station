CFLAGS=-Wall -g -O2
REMOTE_HOST=pi

default:
	gcc ${CFLAGS} -o merger merger.c server.c sender.c timeout.c dict.c http.c config.c -lpthread

upload:
	ssh ${REMOTE_HOST} 'cd rfid-base-station; git pull; cd merger; make; make testclient'

testclient:
	gcc ${CFLAGS} -o testclient testclient.c

test: testclient
	./merger >merger.log 2>&1 &
	ssh ${REMOTE_HOST} ./testclient &
	./testclient &
	tail -f merger.log
