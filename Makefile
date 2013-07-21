everything:
	mkdir -p build
	make -C receive
	make -C merger
	(cd ./deploy/; sudo ./deploy.sh)
