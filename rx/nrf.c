#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <stdio.h>

uchar TX_ADDRESS[TX_ADR_WIDTH] = {0xff,0xff,0xff,0xff,0xff};//本地地址
uchar RX_ADDRESS[TX_ADR_WIDTH] = {0xff,0xff,0xff,0xff,0xff};//本地地址

static uchar station_R, station_T;

#define RX_DR	sta&(1<<6)
#define TX_DS	sta&(1<<5)
#define MAX_RT 	sta&(1<<4)

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

void SetRX_Mode(void)
{
	CE(0); // 拉低CE，进入工作模式的选择
    SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH);
    SPI_write_reg(WRITE_REG + RF_CH, station_R);//设置接收频道
	SPI_write_reg(WRITE_REG + CONFIG, 0x0f); // 配置：IRQ收发完成中断响应，16位CRC ，接收模式
	CE(1); // 置高CE，切换工作模式
    usleep(500); // 切换模式后至少130us,见数据手册
}

/* @brief   receive packet from nRF24L01
 * @param   rx_buf  receive buffer
 * @return  whether or not received
 */
bool nRF24L01_RxPacket(uchar* rx_buf)
{
	bool received = false;
    uchar sta;
	CE(0); // NRF24C01使能
	sta = SPI_Read(STATUS); // 读取状态寄存器接受标志
	if (sta & 0x40) // 判断是否接收到数据
	{
		SPI_Read_Buf(RD_RX_PLOAD,rx_buf,RX_PLOAD_WIDTH); //接收数据
		received = true; //读取数据完成标志
	}
	SPI_write_reg(FLUSH_RX,0); //清空接收数据缓存区
	SPI_write_reg(WRITE_REG+STATUS, sta); //接收到数据后，通过写1来清除中断标志
	CE(1); // 置高CE，切换工作模式
	return received;
}

void nRF24L01_TxPacket(uchar *BUF)
{
    CE(0);
    SPI_write_reg(WRITE_REG + RF_CH,0);//设置工作信道为0
    SPI_write_reg(FLUSH_TX, 0);//清空发送数据缓冲区
    
    SPI_Read(READ_REG+FIFO_STATUS);
    
    SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);//写接收端地址
    SPI_Write_Buf(WR_TX_PLOAD, BUF, TX_PLOAD_WIDTH);//装载数据
    
    //IRQ不屏蔽完成中断响应，16位CRC，发送模式
    SPI_write_reg(WRITE_REG + CONFIG, 0x0e);
    CE(1);//置高CE，数据开始发送
    usleep(20);//等待数据发送完成
}

void init_NRF24L01(uchar station)
{
	station_T=station;
    station_R=station;
	CE(0); // 拉低CE,NRF24C01使能
	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH); // 写本地地址
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, TX_ADR_WIDTH); // 装载通道0的地址，用于ACK
//*********************************配置NRF24L01**************************************
    SPI_write_reg(WRITE_REG + STATUS, 0x70); //清中断标志位
	SPI_write_reg(WRITE_REG + EN_AA, 0x00); //ACK自动应答0通道不允许
	SPI_write_reg(WRITE_REG + EN_RXADDR, 0x01); // 允许接收地址只有频道0
	SPI_write_reg(WRITE_REG + SETUP_RETR, 0x00); //取消自动重发功能
	SPI_write_reg(WRITE_REG + RF_CH, station_R); // 设置信道工作为2.4GHZ，收发必须一致
	SPI_write_reg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH); //设置接收数据长度
	SPI_write_reg(WRITE_REG + RF_SETUP, 0x07); //设置发射速率为1MHZ，发射功率为最大值0dB
    SPI_write_reg(WRITE_REG + CONFIG, 0x0f); // IRQ收发完成中断响应，16位CRC 
    flush_rx();
    CE(1);
	usleep(10000); //初始化完成
}
