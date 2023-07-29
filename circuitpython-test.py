import board
from busio import I2C

i2c = I2C(board.GP3, board.GP2)
i2c.try_lock()

wbuf = bytearray("\x00abcdabcdabcdabcdabcdabcdabcdabcd", "ascii")
wbuf[0] = 0x00
i2c.writeto(0x09, wbuf)
rbuf = bytearray(32)
i2c.readfrom_into(0x09, rbuf)
print(rbuf)

i2c.unlock()
