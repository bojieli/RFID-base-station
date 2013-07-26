#!/bin/bash
# install service to Debian GNU/Linux
# see http://gitlab.lug.ustc.edu.cn/gewuit/rfid-base-station/wikis/home

if [ -z "$1" ]; then
    echo "Usage: ./deploy.sh [ master <access-token> | slave <ip-of-master> | update ]"
    exit 1
elif [ "$1" == "update" ]; then
    ACTION=update
    # trick: if merger is to be started at boot, then it is master
    if [ -f "/etc/rc2.d/S19merger" ]; then
        TARGET=master
    else
        TARGET=slave
    fi
elif [ "$1" == "master" ]; then
    if [ -z "$2" ]; then
        echo "Please specify access token"
        exit 1
    fi
    ACTION=install
    TARGET=$1
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
    ACTION=install
    TARGET=$1
    MASTER_IP=$2
    ACCESS_TOKEN="slave-fake-token"
else
    echo "Please specify master, slave or update as first parameter"
    exit 1
fi

echo "$ACTION for $TARGET."
if [ "$ACTION" == "install" ]; then
    echo "master ip: $MASTER_IP"
    echo "access token: $ACCESS_TOKEN"
fi

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

# generate config files
if [ "$ACTION" == "install" ]; then
    cp ../config/* $INSTALL_DIR/etc/
    MERGER_CONF=$INSTALL_DIR/etc/merger.ini
    sed -i "s/^listen.local_ip = .*$/listen.local_ip = $MASTER_IP/" $MERGER_CONF
    sed -i "s/^cloud.access_token = .*$/cloud.access_token = $ACCESS_TOKEN/" $MERGER_CONF
    RECEIVER_CONF=$INSTALL_DIR/etc/receiver.ini
    sed -i "s/^master.ip = .*$/master.ip = $MASTER_IP/" $RECEIVER_CONF
fi

# install init scripts
cp init.d/{merger,receiver} /etc/init.d/
if [ "$ACTION" == "install" ]; then
    for i in {2..5}; do
        rm -f /etc/rc${i}.d/S19merger /etc/rc${i}.d/S20receiver
        if [ "$TARGET" == "master" ]; then 
            # merger must be started before receiver
            ln -s /etc/init.d/merger /etc/rc${i}.d/S19merger
        fi
        ln -s /etc/init.d/receiver /etc/rc${i}.d/S20receiver
    done
fi

# start services
if [ "$TARGET" == "master" ]; then
    /etc/init.d/merger start
fi
/etc/init.d/receiver start

