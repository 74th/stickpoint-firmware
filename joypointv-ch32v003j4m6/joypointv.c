#include "ch32v003fun.h"
#include "joypointv.h"
#include <stdio.h>

// The I2C slave library uses a one byte address so you can extend the size of this array up to 256 registers
// note that the register set is modified by interrupts, to prevent the compiler from accidently optimizing stuff
// away make sure to declare the register array volatile

volatile uint8_t i2c_registers[32] = {0x00};

void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt));

void I2C1_EV_IRQHandler(void)
{
    uint16_t STAR1, STAR2 __attribute__((unused));
    STAR1 = I2C1->STAR1;
    STAR2 = I2C1->STAR2;

    printf("EV STAR1: 0x%04x STAR2: 0x%04x\r\n", STAR1, STAR2);

    I2C1->CTLR1 |= I2C_CTLR1_ACK;

    if (STAR1 & I2C_STAR1_ADDR) // 0x0002
    {
        printf("ADDR\r\n");
        // 最初のイベント
        // read でも write でも必ず最初に呼ばれる
        i2c_slave_state.first_write = 1;
        i2c_slave_state.position = i2c_slave_state.offset;
    }

    if (STAR1 & I2C_STAR1_RXNE) // 0x0040
    {
        printf("RXNE write event: pos:%d\r\n", i2c_slave_state.position);
        // 1byte の write イベント（master -> slave）
        if (i2c_slave_state.first_write)
        {
            // 1byte 受信
            // オフセットとして設定
            i2c_slave_state.offset = I2C1->DATAR;
            i2c_slave_state.position = i2c_slave_state.offset;
            i2c_slave_state.first_write = 0;
        }
        else
        {
            if (i2c_slave_state.position < i2c_slave_state.size)
            {
                // 1byte 受信
                i2c_slave_state.registers[i2c_slave_state.position] = I2C1->DATAR;
                i2c_slave_state.position++;
            }
            else
            {
                // 必ず読み込まないと TIMEOUT してしまう
                I2C1->DATAR;
            }
        }
    }

    if (STAR1 & I2C_STAR1_TXE) // 0x0080
    {
        // 1byte の read イベント（slave -> master）
        printf("TXE write event: pos:%d\r\n", i2c_slave_state.position);
        if (i2c_slave_state.position < i2c_slave_state.size)
        {
            // 1byte 送信
            I2C1->DATAR = i2c_slave_state.registers[i2c_slave_state.position];
            i2c_slave_state.position++;
        }
        else
        {
            // 1byte 送信
            I2C1->DATAR = 0x00;
        }
    }
    //
}

void I2C1_ER_IRQHandler(void)
{
    uint16_t STAR1 = I2C1->STAR1;

    printf("ER STAR1: 0x%04x\r\n", STAR1);

    if (STAR1 & I2C_STAR1_BERR)           // 0x0100
    {                                     // Bus error
        I2C1->STAR1 &= ~(I2C_STAR1_BERR); // Clear error
    }

    if (STAR1 & I2C_STAR1_ARLO)           // 0x0200
    {                                     // Arbitration lost error
        I2C1->STAR1 &= ~(I2C_STAR1_ARLO); // Clear error
    }

    if (STAR1 & I2C_STAR1_AF)           // 0x0400
    {                                   // Acknowledge failure
        I2C1->STAR1 &= ~(I2C_STAR1_AF); // Clear error
    }
}

int main()
{
    SystemInit();

    printf("using ch32v003fun\r\n");

    printf("initialize\r\n");

    SetupI2CSlave(0x9, i2c_registers, sizeof(i2c_registers));

    // Enable GPIOD and set pin 0 to output
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
    GPIOD->CFGLR &= ~(0xf << (4 * 0));
    GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0);

    printf("initialize done\r\n");

    while (1)
    {
        if (i2c_registers[0] & 1)
        { // Turn on LED (PD0) if bit 1 of register 0 is set
            GPIOD->BSHR |= 1 << 16;
        }
        else
        {
            GPIOD->BSHR |= 1;
        }
    }
}
