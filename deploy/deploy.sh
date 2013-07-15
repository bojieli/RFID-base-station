#!/bin/bash
if [ `whoami` != 'root' ]; then
    echo "You must be root!"
    exit 1
fi
if [ `dirname $0` != '.' ]; then
    echo "You must run the script in its directory!"
    exit 1
fi

mv /etc/rc.local /etc/rc.local.old
ln -s `pwd`/rc.local /etc/rc.local
