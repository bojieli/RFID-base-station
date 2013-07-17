#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <stdio.h>

#define RX_DR	sta&(1<<6)
#define TX_DS	sta&(1<<5)
#define MAX_RT 	sta&(1<<4)

/* @brief   write a byte to SPI channel 0
 * @param   value: value
 */
void SPI_write_byte(uchar value)
{
    wiringPiSPIDataRW(0, &value, 1);
}

/* @brief   read a byte from SPI channel 0
 * @return  the byte read
 */
uchar SPI_read_byte()
{
    uchar value = 0xCC;
    wiringPiSPIDataRW(0, &value, 1); 
    return value;
}

uchar SPI_Read(uchar reg)
{
	uchar reg_val;
	CSN(0);
	SPI_write_byte(reg);//发送地址
	reg_val = SPI_read_byte();//读取从地址返回的数据
	CSN(1);
	return(reg_val);//返回接收的数据
}

static void SPI_write_reg(uchar reg, uchar value)
{
    CSN(0);
    SPI_write_byte(reg);//select register
    SPI_write_byte(value);//write value to it
    CSN(1);
}

// 由于只在一处被调用，使用 inline
static inline void SPI_write_buf(uchar reg, uchar * pBuf, uchar bytes)
{
    CSN(0);
    SPI_write_byte(reg);//发送要写入数据的地址
    uchar i;
    for(i=0; i<bytes; i++)
        SPI_write_byte(*pBuf++);//写入数据
    CSN(1);
}

void nRF24L01_TxPacket(uchar *buf)
{
    CE(0);
    SPI_write_buf(WR_TX_PAYLOAD_NOACK, buf, TX_PLOAD_WIDTH);//装载数据
    CE(1);//置高CE，数据开始发送
    usleep(20);
}

// 写本地地址，TX_ADR_WIDTH 个字节全为 0xFF
// 为节省空间而优化
static inline void write_local_addr(void)
{
    CSN(0);
    SPI_write_byte(WRITE_REG + TX_ADDR);
    uchar i;
    for (i=0; i<TX_ADR_WIDTH; i++)
        SPI_write_byte(0xFF);
    CSN(1);
}

void init_NRF24L01(uchar station)
{
    CE(0);  //拉低CE,NRF24C01使能
    write_local_addr();
//*********************************配置NRF24L01**************************************
    SPI_write_reg(WRITE_REG + STATUS, 0x70); //清中断标志位
    SPI_write_reg(WRITE_REG + EN_AA, 0x01);               //ACK自动应答0通道允许,
    SPI_write_reg(WRITE_REG + EN_RXADDR, 0x00);           //允许接收地址只有频道0
    SPI_write_reg(WRITE_REG + SETUP_RETR, 0x00);          //取消自动重发功能
    SPI_write_reg(WRITE_REG + RF_CH, station);            //设置频道
    SPI_write_reg(WRITE_REG + RF_SETUP, 0x07);            //设置发射速率为1MHZ，发射功率为最大值0dB
    SPI_write_reg(WRITE_REG + CONFIG, 0x7e);              //IRQ收发完成中断响应，16位CRC
    SPI_write_reg(WRITE_REG + ACTIVATE, 0x73);
    SPI_write_reg(WRITE_REG + FEATURE, 0x01);             //enable W_ＴX_PALOAD_NOACK
    SPI_write_reg(FLUSH_TX, 0);//清空发送数据缓冲区
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
