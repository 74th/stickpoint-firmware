import board
from busio import I2C
import time


ADDRESS = 0x0A

def run():
    try:
        i2c = I2C(board.GP3, board.GP2)
        i2c.try_lock()
        rbuf = bytearray(5)
        while True:
            i2c.readfrom_into(ADDRESS, rbuf)
            print(f"left:{rbuf[0]:3d} right:{rbuf[1]:3d} down:{rbuf[2]:3d} up:{rbuf[3]:3d}")
            time.sleep(0.5)
    finally:
        i2c.unlock()
        i2c.deinit()

def scan():
    try:
        i2c = I2C(board.GP3, board.GP2)
        i2c.try_lock()
        print(i2c.scan())
    finally:
        i2c.unlock()
        i2c.deinit()

run()
