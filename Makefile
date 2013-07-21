everything:
	mkdir -p build
	make -C receive
	make -C merger
	sudo ./deploy/deploy.sh
