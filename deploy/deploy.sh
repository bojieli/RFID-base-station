#!/bin/bash

mv /etc/rc.local /etc/rc.local.old
ln -s `dirname $0`/rc.local /etc/rc.local
