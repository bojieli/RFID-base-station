everything:
	@mkdir -p build
	make -C receive
	make -C merger
	make -C region-test
	sudo ./deploy/deploy.sh update
