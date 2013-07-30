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
    echo "slave ip: $SLAVE_IP"
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
cp -a $CODE_BASE/build/* $INSTALL_DIR/bin/

# generate config files
if [ "$ACTION" == "install" ]; then
    cp -a $CODE_BASE/config/* $INSTALL_DIR/etc/
    MERGER_CONF=$INSTALL_DIR/etc/merger.ini
    sed -i "s/^listen.local_ip = .*$/listen.local_ip = $MASTER_IP/" $MERGER_CONF
    sed -i "s/^cloud.access_token = .*$/cloud.access_token = $ACCESS_TOKEN/" $MERGER_CONF
    RECEIVER_CONF=$INSTALL_DIR/etc/receiver.ini
    sed -i "s/^master.ip = .*$/master.ip = $MASTER_IP/" $RECEIVER_CONF
fi

# install init scripts
cp -a $CODE_BASE/deploy/init.d/{merger,receiver} /etc/init.d/
if [ "$ACTION" == "install" ]; then
    if [ "$TARGET" == "master" ]; then 
        insserv -d /etc/init.d/merger # fix possible warnings
        update-rc.d merger defaults
    else ## merger is not enabled on slave, so remove the dependency
        sed -i 's/\(Required-\(Start\|Stop\)\:.*\)merger/\1/' /etc/init.d/receiver
    fi
    insserv -d /etc/init.d/receiver
    update-rc.d receiver defaults
fi

# install helper scripts
cp -a $CODE_BASE/deploy/helper/* /usr/local/bin/

# set hostname
if [ "$ACTION" == "install" ]; then
    echo $TARGET > /etc/hostname
    hostname $TARGET
    sed -i '/^127\.0\.0\.1.*/d' /etc/hosts
    sed -i '/\(master\|slave\)$/d' /etc/hosts
    echo "127.0.0.1 localhost" >> /etc/hosts
    echo "$MASTER_IP master" >> /etc/hosts
    echo "$SLAVE_IP slave" >> /etc/hosts
fi

# start services
if [ "$TARGET" == "master" ]; then
    /etc/init.d/merger start
fi
/etc/init.d/receiver start

