#include "ch32v003fun.h"
#include "joypointv.h"
#include <stdio.h>

#define ADC_NUMCHLS 3

volatile uint8_t i2c_registers[32] = {0x00};
volatile uint16_t adc_buffer[ADC_NUMCHLS];

void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt));

void init_rcc(void)
{
    RCC->CFGR0 &= ~(0x1F << 11);

    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;
}

void init_i2c_slave(uint8_t address, volatile uint8_t *registers, uint8_t size)
{
    // https://github.com/cnlohr/ch32v003fun/blob/master/examples/i2c_slave/i2c_slave.h
    i2c_slave_state.first_write = 1;
    i2c_slave_state.offset = 0;
    i2c_slave_state.position = 0;
    i2c_slave_state.registers = registers;
    i2c_slave_state.size = size;

    // PC1 is SDA, 10MHz Output, alt func, open-drain
    GPIOC->CFGLR &= ~(0xf << (4 * 1));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF) << (4 * 1);

    // PC2 is SCL, 10MHz Output, alt func, open-drain
    GPIOC->CFGLR &= ~(0xf << (4 * 2));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF) << (4 * 2);

    // Reset I2C1 to init all regs
    RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

    I2C1->CTLR1 |= I2C_CTLR1_SWRST;
    I2C1->CTLR1 &= ~I2C_CTLR1_SWRST;

    // Set module clock frequency
    uint32_t prerate = 2000000; // I2C Logic clock rate, must be higher than the bus clock rate
    I2C1->CTLR2 |= (APB_CLOCK / prerate) & I2C_CTLR2_FREQ;

    // Enable interrupts
    I2C1->CTLR2 |= I2C_CTLR2_ITBUFEN;
    I2C1->CTLR2 |= I2C_CTLR2_ITEVTEN; // Event interrupt
    I2C1->CTLR2 |= I2C_CTLR2_ITERREN; // Error interrupt

    NVIC_EnableIRQ(I2C1_EV_IRQn); // Event interrupt
    NVIC_SetPriority(I2C1_EV_IRQn, 2 << 4);
    NVIC_EnableIRQ(I2C1_ER_IRQn); // Error interrupt
    // Set clock configuration
    uint32_t clockrate = 1000000;                                                    // I2C Bus clock rate, must be lower than the logic clock rate
    I2C1->CKCFGR = ((APB_CLOCK / (3 * clockrate)) & I2C_CKCFGR_CCR) | I2C_CKCFGR_FS; // Fast mode 33% duty cycle
    // I2C1->CKCFGR = ((APB_CLOCK/(25*clockrate))&I2C_CKCFGR_CCR) | I2C_CKCFGR_DUTY | I2C_CKCFGR_FS; // Fast mode 36% duty cycle
    // I2C1->CKCFGR = (APB_CLOCK/(2*clockrate))&I2C_CKCFGR_CCR; // Standard mode good to 100kHz

    // Set I2C address
    I2C1->OADDR1 = address << 1;

    // Enable I2C
    I2C1->CTLR1 |= I2C_CTLR1_PE;

    // Acknowledge the first address match event when it happens
    I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

void init_adc(void)
{

    // ADCCLK = 24 MHz => RCC_ADCPRE = 0: divide by 2
    RCC->CFGR0 &= ~(0x1F << 11);

    // Enable GPIOD and ADC
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                      RCC_APB2Periph_ADC1;

    // A0 PA2
    GPIOA->CFGLR &= ~(0xf << (4 * 2)); // CNF = 00: Analog, MODE = 00: Input

    // A1 PA1
    GPIOA->CFGLR &= ~(0xf << (4 * 2)); // CNF = 00: Analog, MODE = 00: Input

    // A2 PA4
    GPIOC->CFGLR &= ~(0xf << (4 * 4)); // CNF = 00: Analog, MODE = 00: Input

    // Reset the ADC to init all regs
    RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

    // Set up four conversions on chl 0, 1, 2
    ADC1->RSQR1 = (ADC_NUMCHLS - 1) << 20; // four chls in the sequence
    ADC1->RSQR2 = 0;
    ADC1->RSQR3 = (0 << (5 * 0)) | (1 << (5 * 1)) | (2 << (5 * 2));

    // set sampling time for chl 7, 4, 3, 2
    // 0:7 => 3/9/15/30/43/57/73/241 cycles
    ADC1->SAMPTR2 = (7 << (3 * 0)) | (7 << (3 * 1)) | (7 << (3 * 2));

    // turn on ADC
    ADC1->CTLR2 |= ADC_ADON;

    // Reset calibration
    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL)
        ;

    // Calibrate
    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL)
        ;

    // Turn on DMA
    RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

    // DMA1_Channel1 is for ADC
    DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
    DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
    DMA1_Channel1->CNTR = ADC_NUMCHLS;
    DMA1_Channel1->CFGR =
        DMA_M2M_Disable |
        DMA_Priority_VeryHigh |
        DMA_MemoryDataSize_HalfWord |
        DMA_PeripheralDataSize_HalfWord |
        DMA_MemoryInc_Enable |
        DMA_Mode_Circular |
        DMA_DIR_PeripheralSRC;

    // Turn on DMA channel 1
    DMA1_Channel1->CFGR |= DMA_CFGR1_EN;

    // enable scanning
    ADC1->CTLR1 |= ADC_SCAN;

    // Enable continuous conversion and DMA
    ADC1->CTLR2 |= ADC_CONT | ADC_DMA | ADC_EXTSEL;

    // start conversion
    ADC1->CTLR2 |= ADC_SWSTART;
}

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
    init_rcc();

    printf("using ch32v003fun\r\n");
    printf("initialize\r\n");

    init_i2c_slave(0x9, i2c_registers, sizeof(i2c_registers));
    init_adc();

    printf("initialize done\r\n");

    while (1)
    {
        printf("%4d %4d %4d\n\r", adc_buffer[0], adc_buffer[1], adc_buffer[2]);
        Delay_Ms(500);
    }
}
