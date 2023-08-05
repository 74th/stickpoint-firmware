all : flash

CH32V003FUN=./ch32v003fun
MINICHLINK=minichlink
TARGET:=stickpointv

include ./ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean

upload_test:
	sudo mount /dev/sda1 /media/CIRCUITPY -o uid=`id -u`,gid=`id -g`
	cp ../circuitpython-test.py /media/CIRCUITPY/joypoint.py
	sync
	sudo umount /dev/sda1
