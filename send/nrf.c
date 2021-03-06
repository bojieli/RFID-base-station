#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <stdio.h>

uchar TX_ADDRESS[TX_ADR_WIDTH] = {0xff,0xff,0xff,0xff,0xff};//本地地址

#define RX_DR	sta&(1<<6)
#define TX_DS	sta&(1<<5)
#define MAX_RT 	sta&(1<<4)

#define SLEEP_TIMEOUT 800

/* @brief   write a byte to SPI channel 0
 * @param   value: value
 */
void spi_write_byte(uchar value)
{
    wiringPiSPIDataRW(0, &value, 1);
}

/* @brief   read a byte from SPI channel 0
 * @return  the byte read
 */
uchar spi_read_byte()
{
    uchar value = 0xCC;
    wiringPiSPIDataRW(0, &value, 1); 
    return value;
}

// write register
void SPI_write_reg(uchar reg, uchar value)
{
    CSN(0);
    spi_write_byte(reg);
    spi_write_byte(value);
    CSN(1);
}

// read one byte
uchar SPI_Read(uchar reg)
{
	uchar reg_val;
	CSN(0);
	spi_write_byte(reg);//发送地址
	reg_val = spi_read_byte();//读取从地址返回的数据
	CSN(1);
	return(reg_val);//返回接收的数据
}

// read multiple bytes
void SPI_Read_Buf(uchar reg, uchar* buf, uchar bytes)
{
	CSN(0);
	spi_write_byte(reg);//发送要读出数据的地址
    wiringPiSPIDataRW(0, buf, bytes); 
	CSN(1);
}

// write multiple bytes
void SPI_Write_Buf(uchar reg, uchar* buf, uchar bytes)
{
	CSN(0);//SPI enable
	spi_write_byte(reg);//发送要写入数据的地址
    wiringPiSPIDataRW(0, buf, bytes);
	CSN(1);//SPI disable
}

void flush_rx()
{
    CSN(0);
    SPI_write_reg(FLUSH_RX, 0);
    CSN(1);
}

void nRF24L01_TxPacket(uchar *BUF)
{
    CE(0);
    SPI_write_reg(WRITE_REG + STATUS, 0x70); // clear intr
    SPI_write_reg(FLUSH_TX, 0);//清空发送数据缓冲区
    SPI_Write_Buf(WR_TX_PLOAD, BUF, TX_PLOAD_WIDTH);//装载数据
    CE(1);//置高CE，数据开始发送
    usleep(SLEEP_TIMEOUT);//等待数据发送完成
}

void init_NRF24L01(uchar station)
{
	CE(0); // 拉低CE,NRF24C01使能
	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH); // 写本地地址
//*********************************配置NRF24L01**************************************
    SPI_write_reg(WRITE_REG + STATUS, 0x70); //清中断标志位
	SPI_write_reg(WRITE_REG + EN_AA, 0x01); //ACK自动应答0通道不允许
	SPI_write_reg(WRITE_REG + EN_RXADDR, 0x01); // 允许接收地址只有频道0
	SPI_write_reg(WRITE_REG + SETUP_RETR, 0x00); //取消自动重发功能
	SPI_write_reg(WRITE_REG + RF_CH, station); // 设置信道工作为2.4GHZ，收发必须一致
	SPI_write_reg(WRITE_REG + RF_SETUP, 0x07); //设置发射速率为1MHZ，发射功率为最大值0dB
    SPI_write_reg(WRITE_REG + CONFIG, 0x7e); // IRQ发送模式
    CE(1);
	usleep(10000); //初始化完成
}

void print_configs()
{
    CE(0);
#define PCONF(x) printf("%15s   0x%02x\n", #x, SPI_Read(x))
    PCONF(STATUS);
    PCONF(EN_AA);
    PCONF(EN_RXADDR);
    PCONF(SETUP_RETR);
    PCONF(RF_CH);
    PCONF(RX_PW_P0);
    PCONF(RF_SETUP);
    PCONF(CONFIG);
#undef PCONF
    CE(1);
}
