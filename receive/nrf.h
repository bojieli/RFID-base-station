#ifndef  __nRF24L01__
#define  __nRF24L01__

#include "common.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>

extern int IRQ_PIN, CSN_PIN, CE_PIN, LED_PIN, LED2_PIN;

extern int TX_ADR_WIDTH, RX_ADR_WIDTH;
extern uchar* TX_ADDRESS, RX_ADDRESS;

extern int TX_PLOAD_WIDTH, RX_PLOAD_WIDTH;
#define BUF_SIZE RX_PLOAD_WIDTH

#define CSN(state)  digitalWrite(CSN_PIN, state)
#define CE(state)   digitalWrite(CE_PIN, state)

// SPI(nRF24L01) commands
#define READ_REG    0x00  // Define read command to register，读寄存器指令
#define WRITE_REG   0x20  // Define write command to register，写寄存器指令
#define RD_RX_PLOAD 0x61  // Define RX payload register address，读取接收数据指令
#define WR_TX_PLOAD 0xA0  // Define TX payload register address，写待发数据指令
#define FLUSH_TX    0xE1  // Define flush TX register command，冲洗发送FIFO指令
#define FLUSH_RX    0xE2  // Define flush RX register command，冲洗接收FIFO指令
#define REUSE_TX_PL 0xE3  // Define reuse TX payload register command，定义重复装载数据指令
#define NOP         0xFF  // Define No Operation, might be used to read status register，保留

#define CONFIG      0x00  // 'Config' register address，配置收发状态，CRC校验模式及收发状态响应方式
#define EN_AA       0x01  // 'Enable Auto Acknowledgment' register address，自动应答功能设置
#define EN_RXADDR   0x02  // 'Enabled RX addresses' register address，可用信道设置
#define SETUP_AW    0x03  // 'Setup address width' register address，收发地址宽度设置
#define SETUP_RETR  0x04  // 'Setup Auto. Retrans' register address，自动重发功能设置
#define RF_CH       0x05  // 'RF channel' register address，工作频率设置
#define RF_SETUP    0x06  // 'RF setup' register address，发送速率，功耗设置
#define STATUS      0x07  // 'Status' register address，状态寄存器
#define OBSERVE_TX  0x08  // 'Observe TX' register address，发送监测功能
#define CD          0x09  // 'Carrier Detect' register address，地址监测
#define RX_ADDR_P0  0x0A  //频道0接收数据地址
#define RX_ADDR_P1  0x0B  // 'RX address pipe1' register address，
#define RX_ADDR_P2  0x0C  // 'RX address pipe2' register address
#define RX_ADDR_P3  0x0D  // 'RX address pipe3' register address
#define RX_ADDR_P4  0x0E  // 'RX address pipe4' register address
#define RX_ADDR_P5  0x0F  // 'RX address pipe5' register address
#define TX_ADDR     0x10  // 'TX address' register address
#define RX_PW_P0    0x11  //
#define RX_PW_P1    0x12  // 'RX payload width, pipe1' register address
#define RX_PW_P2    0x13  // 'RX payload width, pipe2' register address
#define RX_PW_P3    0x14  // 'RX payload width, pipe3' register address
#define RX_PW_P4    0x15  // 'RX payload width, pipe4' register address
#define RX_PW_P5    0x16  // 'RX payload width, pipe5' register address
#define FIFO_STATUS 0x17  // 'FIFO Status Register' register address

bool nRF24L01_RxPacket(uchar* rx_buf);
void nRF24L01_TxPacket(uchar* buf);
void SetRX_Mode(void);
void init_NRF24L01(uchar station);
void print_configs(void);

#endif
