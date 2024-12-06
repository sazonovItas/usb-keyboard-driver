#!/usr/bin/env bash

DRIVER_NAME=${DRIVER_NAME:=usbkbd}
if [ ! -z "$(lsmod | grep "^$DRIVER_NAME")" ]; then
  sudo rmmod $DRIVER_NAME
fi

make && sudo insmod "${DRIVER_NAME}.ko"
make clean
