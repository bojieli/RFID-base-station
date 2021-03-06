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
    echo "Usage: ./deploy.sh [ master <access-token> | slave <access-token> | update ] [<vendor>]"
    exit 1
elif [ "$1" == "update" ]; then
    ACTION=update
    if [ `hostname` == "master" ]; then
        TARGET=master
    else
        TARGET=slave
    fi
    VENDOR=${2:-default}
elif [ "$1" == "master" ]; then
    if [ -z "$2" ]; then
        echo "Please specify access token"
        exit 1
    fi
    ACTION=install
    TARGET=$1
    ACCESS_TOKEN=$2
    VENDOR=${3:-default}
elif [ "$1" == "slave" ]; then
    if [ -z "$2" ]; then
        echo "Please specify access token"
        exit 1
    fi
    ACTION=install
    TARGET=$1
    ACCESS_TOKEN=$2
    VENDOR=${3:-default}
else
    echo "Please specify master, slave or update as first parameter"
    exit 1
fi

echo "$ACTION for $TARGET."
if [ "$ACTION" == "install" ]; then
    echo "access token: $ACCESS_TOKEN"
    echo "vendor: $VENDOR"
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
    # preserve file owner for easy editing of configs
    if [ "$VENDOR" != "default" ]; then
        cp -a $CODE_BASE/config/$VENDOR/* $INSTALL_DIR/etc/
    else
        cp -a $CODE_BASE/config/* $INSTALL_DIR/etc/
    fi
    MERGER_CONF=$INSTALL_DIR/etc/merger.ini
    sed -i "s/^cloud.access_token = .*$/cloud.access_token = $ACCESS_TOKEN/" $MERGER_CONF
    RECEIVER_CONF=$INSTALL_DIR/etc/receiver.ini
    sed -i "s/^cloud.access_token = .*$/cloud.access_token = $ACCESS_TOKEN/" $RECEIVER_CONF
fi

# install init scripts
cp $CODE_BASE/deploy/init.d/{merger,receiver} /etc/init.d/
if [ "$ACTION" == "install" ]; then
    rm -f /etc/rc{0,1,2,3,4,5,6,S}.d/*{merger,receiver}* # do cleanup
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
cp $CODE_BASE/deploy/usr-local-bin/* /usr/local/bin/

# enable SPI kernel module
sed -i '/^blacklist spi-bcm2708/d' /etc/modprobe.d/raspi-blacklist.conf

# install logrotate
# logrotate configs must be owned by root
cp $CODE_BASE/deploy/logrotate.hourly.conf /etc/
chown root /etc/logrotate.hourly.conf
tmpfile="/tmp/root_crontab"
crontab -l >$tmpfile
sed -i '/\/usr\/sbin\/logrotate \/etc\/logrotate.hourly.conf/d' $tmpfile
sed -i '/\/usr\/local\/bin\/upload-logs/d' $tmpfile
sed -i '/\/usr\/local\/bin\/ecard-watchdog/d' $tmpfile
sed -i '/\/usr\/local\/bin\/report-local-ip/d' $tmpfile
echo "0 * * * *  /usr/sbin/logrotate /etc/logrotate.hourly.conf" >>$tmpfile
echo "30 * * * *  /usr/local/bin/upload-logs" >>$tmpfile
echo "* * * * *  /usr/local/bin/ecard-watchdog" >>$tmpfile
echo "* * * * *  /usr/local/bin/report-local-ip" >>$tmpfile
crontab $tmpfile
rm $tmpfile

# set hostname
if [ "$ACTION" == "install" ]; then
    echo $TARGET > /etc/hostname
    hostname $TARGET
    sed -i '/^127\.0\.0\.1.*/d' /etc/hosts
    echo "127.0.0.1 localhost" >> /etc/hosts
    echo "127.0.0.1 $TARGET" >> /etc/hosts
fi

# start services
if [ "$TARGET" == "master" ]; then
    /etc/init.d/merger start
fi
/etc/init.d/receiver start

