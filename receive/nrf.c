#include "nrf.h"

static uchar station_R, station_T;

int TX_PLOAD_WIDTH, RX_PLOAD_WIDTH;
int TX_ADR_WIDTH, RX_ADR_WIDTH;

uchar *TX_ADDRESS, *RX_ADDRESS;

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

// write multiple bytes and keep the original buf unchanged
void SPI_Write_Buf_safe(uchar reg, uchar* buf, uchar bytes)
{
    uchar *copied = malloc(bytes);
    memcpy(copied, buf, bytes);
    SPI_Write_Buf(reg, copied, bytes);
    free(copied);
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

static int NRFconf(const char* conf) {
    char full_name[100] = "nrf.";
    strcat(full_name, conf);
    return strtol(get_config(full_name), NULL, 16);
}

void init_NRF24L01(uchar station)
{
    RX_PLOAD_WIDTH = NRFconf("RX_PLOAD_WIDTH");
    TX_PLOAD_WIDTH = RX_PLOAD_WIDTH;
	station_T=station;
    station_R=station;
	CE(0); // 拉低CE,NRF24C01使能
begin:
    SPI_write_reg(WRITE_REG + CONFIG, NRFconf("CONFIG_intr_mask")); // 屏蔽所有中断
    SPI_write_reg(WRITE_REG + STATUS, NRFconf("CONFIG_clear_intr")); //清中断标志位
    SPI_write_reg(WRITE_REG + SETUP_AW, TX_ADR_WIDTH - 2); // 0x01 for 3, 0x02 for 4, 0x03 for 5
    SPI_Write_Buf_safe(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH); // 写本地地址
    SPI_write_reg(WRITE_REG + SETUP_AW, RX_ADR_WIDTH - 2);
	SPI_Write_Buf_safe(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // 装载通道0的地址，用于ACK
//*********************************配置NRF24L01**************************************
	SPI_write_reg(WRITE_REG + EN_AA, NRFconf("EN_AA")); //ACK自动应答0通道不允许
	SPI_write_reg(WRITE_REG + EN_RXADDR, NRFconf("EN_RXADDR")); // 允许接收地址只有频道0
	SPI_write_reg(WRITE_REG + SETUP_RETR, NRFconf("SETUP_RETR")); //取消自动重发功能
	SPI_write_reg(WRITE_REG + RF_CH, station_R); // 设置信道工作为2.4GHZ，收发必须一致
	SPI_write_reg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH); //设置接收数据长度
	SPI_write_reg(WRITE_REG + RF_SETUP, NRFconf("RF_SETUP")); //设置发射速率为1MHZ，发射功率为最大值0dB
    flush_rx();
    SPI_write_reg(WRITE_REG + CONFIG, NRFconf("CONFIG")); // IRQ收发完成中断响应，16位CRC 
	usleep(10000); //初始化完成
    if (SPI_Read(CONFIG) != NRFconf("CONFIG"))
        goto begin;
    CE(1);
}

void print_configs()
{
    CE(0);
    SPI_write_reg(WRITE_REG + STATUS, NRFconf("CONFIG_clear_intr")); //清中断标志位
#define __PCONF(x,data) debug("%15s   0x%02x", #x, data)
#define PCONF(x) __PCONF(#x, SPI_Read(x))
    PCONF(STATUS);
    PCONF(SETUP_AW);
    PCONF(EN_AA);
    PCONF(EN_RXADDR);
    PCONF(SETUP_RETR);
    PCONF(RF_CH);
    PCONF(RX_PW_P0);
    PCONF(RF_SETUP);
    PCONF(CONFIG);

    __PCONF(TX_ADR_WIDTH, TX_ADR_WIDTH);
    __PCONF(RX_ADR_WIDTH, RX_ADR_WIDTH);
    __PCONF(RX_ADDRESS_0, RX_ADDRESS[0]);
    __PCONF(RX_ADDRESS_1, RX_ADDRESS[1]);
    __PCONF(RX_ADDRESS_2, RX_ADDRESS[2]);
#undef __PCONF
#undef PCONF
    fflush(stdout);
    CE(1);
}

#define CHANNEL_MAX 256

static void _switch_channels(bool bitmap[CHANNEL_MAX], int usec)
{
    int channel;
    for (channel = 0; channel < CHANNEL_MAX; channel++) {
        if (bitmap[channel]) {
            init_NRF24L01(channel);
            debug("Initialized channel %d", channel);
            usleep(usec);
        }
    }
}

void cron_channel_switch(void)
{
    int prev_num_channels = 0;
    int prev_last_channel = -1;
    while (true) {
        int usec = atoi(get_config("nrf.channel_switch_interval"));
        if (usec < 10000) // do not allow switch faster than 10ms
            usec = 10000;

        char* channels = get_config("nrf.channel");
        bool bitmap[CHANNEL_MAX] = {0};
        int curr = 0;
        bool isempty = true;
        int num_channels = 0;
        int last_channel = -1;
        while (true) {
            if (*channels >= '0' && *channels <= '9') {
                isempty = false;
                curr = curr * 10 + (*channels - '0');
                if (curr >= CHANNEL_MAX)
                    fatal("invalid channel %d in config", curr);
            }
            else { // separator between channels
                if (!isempty && curr >= 0 && curr < CHANNEL_MAX) {
                    bitmap[curr] = true;
                    ++num_channels;
                    last_channel = curr;
                }
                curr = 0;
                isempty = true;
                if (*channels == '\0')
                    break;
            }
            ++channels;
        }

        if (num_channels == 0) {
            fatal("no available channels");
            usleep(usec);
        }
        else if (num_channels == 1 && prev_num_channels == 1 && last_channel == prev_last_channel) {
            // only one channel and stay the same, no need to switch channel
        }
        else {
            _switch_channels(bitmap, usec);
        }
        prev_num_channels = num_channels;
        prev_last_channel = last_channel;
    }
}
