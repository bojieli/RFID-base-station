everything:
	mkdir -p build
	make -C receive
	make -C merger
	./deploy/deploy.sh
