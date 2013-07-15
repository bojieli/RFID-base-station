#!/bin/bash
if [ `whoami` != 'root' ]; then
    echo "You must be root!"
    exit 1
fi

mv /etc/rc.local /etc/rc.local.old
ln -s `dirname $0`/rc.local /etc/rc.local
