#!/bin/bash
# install service to the machine

if [ `whoami` != 'root' ]; then
    echo "You must be root!"
    exit 1
fi
if [ `dirname $0` != '.' ]; then
    echo "You must run the script in its directory!"
    exit 1
fi

cp init.d/* /etc/init.d/

INSTALL_DIR=/opt/gewuit/rfid
mkdir -p $INSTALL_DIR/{bin,etc,log}

# must stop services before replacing binaries
/etc/init.d/receiver stop
/etc/init.d/merger stop
cp ../build/* $INSTALL_DIR/bin/

cp ../config/* $INSTALL_DIR/etc/

# merger should be started before receiver
/etc/init.d/merger start
/etc/init.d/receiver start

