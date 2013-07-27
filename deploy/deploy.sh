#!/bin/bash
# install service to Debian GNU/Linux
# see http://gitlab.lug.ustc.edu.cn/gewuit/rfid-base-station/wikis/home

isip()
{
    if [[ "$1" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
        return 0
    else
        return 1
    fi
}

if [ -z "$1" ]; then
    echo "Usage: ./deploy.sh [ master <ip-of-slave> <access-token> | slave <ip-of-master> | update ]"
    exit 1
elif [ "$1" == "update" ]; then
    ACTION=update
    if [ `hostname` == "master" ]; then
        TARGET=master
    else
        TARGET=slave
    fi
elif [ "$1" == "master" ]; then
    if ! isip "$2"; then
        echo "slave IP is invalid"
        exit 1
    fi
    if [ -z "$3" ]; then
        echo "Please specify access token"
        exit 1
    fi
    ACTION=install
    TARGET=$1
    MASTER_IP="127.0.0.1"
    SLAVE_IP=$2
    ACCESS_TOKEN=$3
elif [ "$1" == "slave" ]; then
    if ! isip "$2"; then
        echo "master IP is invalid"
        exit 1
    fi
    ACTION=install
    TARGET=$1
    MASTER_IP=$2
    SLAVE_IP="127.0.0.1"
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

CODE_BASE=`dirname $0`/..
INSTALL_DIR=/opt/gewuit/rfid
mkdir -p $INSTALL_DIR/{bin,etc,log}

# must stop services before replacing binaries
/etc/init.d/receiver stop
/etc/init.d/merger stop
cp $CODE_BASE/build/* $INSTALL_DIR/bin/

# generate config files
if [ "$ACTION" == "install" ]; then
    cp $CODE_BASE/config/* $INSTALL_DIR/etc/
    MERGER_CONF=$INSTALL_DIR/etc/merger.ini
    sed -i "s/^listen.local_ip = .*$/listen.local_ip = $MASTER_IP/" $MERGER_CONF
    sed -i "s/^cloud.access_token = .*$/cloud.access_token = $ACCESS_TOKEN/" $MERGER_CONF
    RECEIVER_CONF=$INSTALL_DIR/etc/receiver.ini
    sed -i "s/^master.ip = .*$/master.ip = $MASTER_IP/" $RECEIVER_CONF
fi

# install init scripts
cp $CODE_BASE/deploy/init.d/{merger,receiver} /etc/init.d/
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

# install helper scripts
cp $CODE_BASE/deploy/helper/* /usr/local/bin/

# set hostname
echo $TARGET > /etc/hostname
hostname $TARGET
sed -i '/^127\.0\.0\.1.*/d' /etc/hosts
sed -i '/\(master\|slave\)$/d' /etc/hosts
echo "127.0.0.1 localhost" >> /etc/hosts
echo "$MASTER_IP master" >> /etc/hosts
echo "$SLAVE_IP slave" >> /etc/hosts

# start services
if [ "$TARGET" == "master" ]; then
    /etc/init.d/merger start
fi
/etc/init.d/receiver start

