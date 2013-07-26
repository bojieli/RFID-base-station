#!/bin/bash
# install service to Debian GNU/Linux
# see http://gitlab.lug.ustc.edu.cn/gewuit/rfid-base-station/wikis/home

if [ -z "$1" ]; then
    echo "Usage: ./deploy.sh [ master <access-token> | slave <ip-of-master> ]"
    exit 1
elif [ "$1" == "master" ]; then
    if [ -z "$2" ]; then
        echo "Please specify access token"
        exit 1
    fi
    MASTER_IP="127.0.0.1"
    ACCESS_TOKEN=$2
elif [ "$1" == "slave" ]; then
    if [ -z "$2" ]; then
        echo "Please specify ip address of master"
        exit 1
    elif [[ ! "$2" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
        echo "master IP is invalid"
        exit 1
    fi
    MASTER_IP=$2
    ACCESS_TOKEN="slave-fake-token"
else
    echo "Please specify master or slave as first parameter"
    exit 1
fi
TARGET=$1

echo "Installing for $TARGET."
echo "master ip: $MASTER_IP"
echo "access token: $ACCESS_TOKEN"

if [ `whoami` != 'root' ]; then
    echo "You must be root!"
    exit 1
fi
if [ `dirname $0` != '.' ]; then
    echo "You must run the script in its directory!"
    exit 1
fi

INSTALL_DIR=/opt/gewuit/rfid
mkdir -p $INSTALL_DIR/{bin,etc,log}

# must stop services before replacing binaries
/etc/init.d/receiver stop
/etc/init.d/merger stop
cp ../build/* $INSTALL_DIR/bin/

cp ../config/* $INSTALL_DIR/etc/
MERGER_CONF=$INSTALL_DIR/etc/merger.ini
sed -i "s/^listen.local_ip = .*$/listen.local_ip = $MASTER_IP/" $MERGER_CONF
sed -i "s/^cloud.access_token = .*$/cloud.access_token = $ACCESS_TOKEN/" $MERGER_CONF
RECEIVER_CONF=$INSTALL_DIR/etc/receiver.ini
sed -i "s/^master.ip = .*$/master.ip = $MASTER_IP/" $RECEIVER_CONF

cp init.d/{merger,receiver} /etc/init.d/
if [ "$TARGET" == "master" ]; then 
    # merger must be started before receiver
    ln -s /etc/init.d/merger /etc/rc{2,3,4,5}.d/S19merger
fi
ln -s /etc/init.d/receiver /etc/rc{2,3,4,5}.d/S20receiver

if [ "$TARGET" == "master" ]; then
    /etc/init.d/merger start
fi
/etc/init.d/receiver start

