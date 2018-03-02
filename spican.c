#include "spican.h"

#include "spibus.h"
#include <stdlib.h>

#include <sched.h>
#include <time.h>

#include <stdio.h>

// http://ww1.microchip.com/downloads/en/DeviceDoc/21801d.pdf

#define CANCTRL_REQOP 0xE0
#define CANCTRL_ABAT 0x10
#define CANCTRL_OSM 0x08
#define CANCTRL_CLKEN 0x04
#define CANCTRL_CLKPRE 0x03

#define CANCTRL_REQOP_NORMAL 0x00
#define CANCTRL_REQOP_SLEEP 0x20
#define CANCTRL_REQOP_LOOPBACK 0x40
#define CANCTRL_REQOP_LISTENONLY 0x60
#define CANCTRL_REQOP_CONFIG 0x80
#define CANCTRL_REQOP_POWERUP 0xE0

#define CANSTAT_OPMOD 0xE0
#define CANSTAT_ICOD 0x0E

#define TXB_EXIDE 0x08
#define RTR 0x40

#define RXBnCTRL_RXM_STD 0x20
#define RXBnCTRL_RXM_EXT 0x40
#define RXBnCTRL_RXM_STDEXT 0x00
#define RXBnCTRL_RXM_MASK 0x60
#define RXBnCTRL_RTR 0x08
#define RXB0CTRL_BUKT 0x04

#define CANINTF_RX0IF 0x01
#define CANINTF_RX1IF 0x02

#define SIDH 0
#define SIDL 1
#define EID8 2
#define EID0 3
#define DLC 4
#define DATA 5

#define STAT_RX0IF 0x01
#define STAT_RX1IF 0x02

#define STAT_RXIF_MASK = (STAT_RX0IF | STAT_RX1IF)

#define TXB_ABTF 0x40
#define TXB_MLOA 0x20
#define TXB_TXERR 0x10
#define TXB_TXREQ 0x08
#define TXB_TXIE 0x04
#define TXB_TXP 0x03

#define TXB0 0x30
#define TXB1 0x40
#define TXB2 0x50

#define DLC_MASK 0x0F

#define EFLG_RX1OVR 0x80
#define EFLG_RX0OVR 0x40
#define EFLG_TXBO 0x20
#define EFLG_TXEP 0x10
#define EFLG_RXEP 0x08
#define EFLG_TXWAR 0x04
#define EFLG_RXWAR 0x02
#define EFLG_EWARN 0x01

#define EFLG_ERRORMASK (EFLG_RX1OVR | EFLG_RX0OVR | EFLG_TXBO | EFLG_TXEP | EFLG_RXEP)

#define INSTRUCTION_WRITE 0x02
#define INSTRUCTION_READ 0x03
#define INSTRUCTION_BITMOD 0x05
#define INSTRUCTION_LOAD_TX0 0x40
#define INSTRUCTION_LOAD_TX1 0x42
#define INSTRUCTION_LOAD_TX2 0x44
#define INSTRUCTION_RTS_TX0 0x81
#define INSTRUCTION_RTS_TX1 0x82
#define INSTRUCTION_RTS_TX2 0x84
#define INSTRUCTION_RTS_ALL 0x87
#define INSTRUCTION_READ_RX0 0x90
#define INSTRUCTION_READ_RX1 0x94
#define INSTRUCTION_READ_STATUS 0xA0
#define INSTRUCTION_RX_STATUS 0xB0
#define INSTRUCTION_RESET 0xC0

#define REG_RXF0SIDH 0x00
#define REG_RXF0SIDL 0x01
#define REG_RXF0EID8 0x02
#define REG_RXF0EID0 0x03
#define REG_RXF1SIDH 0x04
#define REG_RXF1SIDL 0x05
#define REG_RXF1EID8 0x06
#define REG_RXF1EID0 0x07
#define REG_RXF2SIDH 0x08
#define REG_RXF2SIDL 0x09
#define REG_RXF2EID8 0x0A
#define REG_RXF2EID0 0x0B
#define REG_CANSTAT 0x0E
#define REG_CANCTRL 0x0F
#define REG_RXF3SIDH 0x10
#define REG_RXF3SIDL 0x11
#define REG_RXF3EID8 0x12
#define REG_RXF3EID0 0x13
#define REG_RXF4SIDH 0x14
#define REG_RXF4SIDL 0x15
#define REG_RXF4EID8 0x16
#define REG_RXF4EID0 0x17
#define REG_RXF5SIDH 0x18
#define REG_RXF5SIDL 0x19
#define REG_RXF5EID8 0x1A
#define REG_RXF5EID0 0x1B
#define REG_TEC 0x1C
#define REG_REC 0x1D
#define REG_RXM0SIDH 0x20
#define REG_RXM0SIDL 0x21
#define REG_RXM0EID8 0x22
#define REG_RXM0EID0 0x23
#define REG_RXM1SIDH 0x24
#define REG_RXM1SIDL 0x25
#define REG_RXM1EID8 0x26
#define REG_RXM1EID0 0x27
#define REG_CNF3 0x28
#define REG_CNF2 0x29
#define REG_CNF1 0x2A
#define REG_CANINTE 0x2B
#define REG_CANINTF 0x2C
#define REG_EFLG 0x2D
#define REG_TXB0CTRL 0x30
#define REG_TXB0SIDH 0x31
#define REG_TXB0SIDL 0x32
#define REG_TXB0EID8 0x33
#define REG_TXB0EID0 0x34
#define REG_TXB0DLC 0x35
#define REG_TXB0DATA 0x36
#define REG_TXB1CTRL 0x40
#define REG_TXB1SIDH 0x41
#define REG_TXB1SIDL 0x42
#define REG_TXB1EID8 0x43
#define REG_TXB1EID0 0x44
#define REG_TXB1DLC 0x45
#define REG_TXB1DATA 0x46
#define REG_TXB2CTRL 0x50
#define REG_TXB2SIDH 0x51
#define REG_TXB2SIDL 0x52
#define REG_TXB2EID8 0x53
#define REG_TXB2EID0 0x54
#define REG_TXB2DLC 0x55
#define REG_TXB2DATA 0x56
#define REG_RXB0CTRL 0x60
#define REG_RXB0SIDH 0x61
#define REG_RXB0SIDL 0x62
#define REG_RXB0EID8 0x63
#define REG_RXB0EID0 0x64
#define REG_RXB0DLC 0x65
#define REG_RXB0DATA 0x66
#define REG_RXB1CTRL 0x70
#define REG_RXB1SIDH 0x71
#define REG_RXB1SIDL 0x72
#define REG_RXB1EID8 0x73
#define REG_RXB1EID0 0x74
#define REG_RXB1DLC 0x75
#define REG_RXB1DATA 0x76

#define N_TXBUFFERS 3
#define N_RXBUFFERS 2



/**************************************
 * Register configs for setting Baud  *
 **************************************/

#define MCP_8MHz_1000kBPS_CFG1 0x00
#define MCP_8MHz_1000kBPS_CFG2 0x80
#define MCP_8MHz_1000kBPS_CFG3 0x80

#define MCP_8MHz_500kBPS_CFG1 0x00
#define MCP_8MHz_500kBPS_CFG2 0x90
#define MCP_8MHz_500kBPS_CFG3 0x82

#define MCP_8MHz_250kBPS_CFG1 0x00
#define MCP_8MHz_250kBPS_CFG2 0xB1
#define MCP_8MHz_250kBPS_CFG3 0x85

#define MCP_8MHz_200kBPS_CFG1 0x00
#define MCP_8MHz_200kBPS_CFG2 0xB4
#define MCP_8MHz_200kBPS_CFG3 0x86

#define MCP_8MHz_125kBPS_CFG1 0x01
#define MCP_8MHz_125kBPS_CFG2 0xB1
#define MCP_8MHz_125kBPS_CFG3 0x85

#define MCP_8MHz_100kBPS_CFG1 0x01
#define MCP_8MHz_100kBPS_CFG2 0xB4
#define MCP_8MHz_100kBPS_CFG3 0x86

#define MCP_8MHz_80kBPS_CFG1 0x01
#define MCP_8MHz_80kBPS_CFG2 0xBF
#define MCP_8MHz_80kBPS_CFG3 0x87

#define MCP_8MHz_50kBPS_CFG1 0x03
#define MCP_8MHz_50kBPS_CFG2 0xB4
#define MCP_8MHz_50kBPS_CFG3 0x86

#define MCP_8MHz_40kBPS_CFG1 0x03
#define MCP_8MHz_40kBPS_CFG2 0xBF
#define MCP_8MHz_40kBPS_CFG3 0x87

#define MCP_8MHz_33k3BPS_CFG1 0x47
#define MCP_8MHz_33k3BPS_CFG2 0xE2
#define MCP_8MHz_33k3BPS_CFG3 0x85

#define MCP_8MHz_31k25BPS_CFG1 0x07
#define MCP_8MHz_31k25BPS_CFG2 0xA4
#define MCP_8MHz_31k25BPS_CFG3 0x84

#define MCP_8MHz_20kBPS_CFG1 0x07
#define MCP_8MHz_20kBPS_CFG2 0xBF
#define MCP_8MHz_20kBPS_CFG3 0x87

#define MCP_8MHz_10kBPS_CFG1 0x0F
#define MCP_8MHz_10kBPS_CFG2 0xBF
#define MCP_8MHz_10kBPS_CFG3 0x87

#define MCP_8MHz_5kBPS_CFG1 0x1F
#define MCP_8MHz_5kBPS_CFG2 0xBF
#define MCP_8MHz_5kBPS_CFG3 0x87

#define MCP_16MHz_1000kBPS_CFG1 0x00
#define MCP_16MHz_1000kBPS_CFG2 0xD0
#define MCP_16MHz_1000kBPS_CFG3 0x82

#define MCP_16MHz_500kBPS_CFG1 0x00
#define MCP_16MHz_500kBPS_CFG2 0xF0
#define MCP_16MHz_500kBPS_CFG3 0x86

#define MCP_16MHz_250kBPS_CFG1 0x41
#define MCP_16MHz_250kBPS_CFG2 0xF1
#define MCP_16MHz_250kBPS_CFG3 0x85

#define MCP_16MHz_200kBPS_CFG1 0x01
#define MCP_16MHz_200kBPS_CFG2 0xFA
#define MCP_16MHz_200kBPS_CFG3 0x87

#define MCP_16MHz_125kBPS_CFG1 0x03
#define MCP_16MHz_125kBPS_CFG2 0xF0
#define MCP_16MHz_125kBPS_CFG3 0x86

#define MCP_16MHz_100kBPS_CFG1 0x03
#define MCP_16MHz_100kBPS_CFG2 0xFA
#define MCP_16MHz_100kBPS_CFG3 0x87

#define MCP_16MHz_80kBPS_CFG1 0x03
#define MCP_16MHz_80kBPS_CFG2 0xFF
#define MCP_16MHz_80kBPS_CFG3 0x87

#define MCP_16MHz_50kBPS_CFG1 0x07
#define MCP_16MHz_50kBPS_CFG2 0xFA
#define MCP_16MHz_50kBPS_CFG3 0x87

#define MCP_16MHz_40kBPS_CFG1 0x07
#define MCP_16MHz_40kBPS_CFG2 0xFF
#define MCP_16MHz_40kBPS_CFG3 0x87

#define MCP_16MHz_33k3BPS_CFG1 0x4E
#define MCP_16MHz_33k3BPS_CFG2 0xF1
#define MCP_16MHz_33k3BPS_CFG3 0x85

#define MCP_16MHz_20kBPS_CFG1 0x0F
#define MCP_16MHz_20kBPS_CFG2 0xFF
#define MCP_16MHz_20kBPS_CFG3 0x87

#define MCP_16MHz_10kBPS_CFG1 0x1F
#define MCP_16MHz_10kBPS_CFG2 0xFF
#define MCP_16MHz_10kBPS_CFG3 0x87

#define MCP_16MHz_5kBPS_CFG1 0x3F
#define MCP_16MHz_5kBPS_CFG2 0xFF
#define MCP_16MHz_5kBPS_CFG3 0x87

#define MCP_20MHz_1000kBPS_CFG1 0x00
#define MCP_20MHz_1000kBPS_CFG2 0xD9
#define MCP_20MHz_1000kBPS_CFG3 0x82

#define MCP_20MHz_500kBPS_CFG1 0x00
#define MCP_20MHz_500kBPS_CFG2 0xFA
#define MCP_20MHz_500kBPS_CFG3 0x87

#define MCP_20MHz_250kBPS_CFG1 0x41
#define MCP_20MHz_250kBPS_CFG2 0xFB
#define MCP_20MHz_250kBPS_CFG3 0x86

#define MCP_20MHz_200kBPS_CFG1 0x01
#define MCP_20MHz_200kBPS_CFG2 0xFF
#define MCP_20MHz_200kBPS_CFG3 0x87

#define MCP_20MHz_125kBPS_CFG1 0x03
#define MCP_20MHz_125kBPS_CFG2 0xFA
#define MCP_20MHz_125kBPS_CFG3 0x87

#define MCP_20MHz_100kBPS_CFG1 0x04
#define MCP_20MHz_100kBPS_CFG2 0xFA
#define MCP_20MHz_100kBPS_CFG3 0x87

#define MCP_20MHz_80kBPS_CFG1 0x04
#define MCP_20MHz_80kBPS_CFG2 0xFF
#define MCP_20MHz_80kBPS_CFG3 0x87

#define MCP_20MHz_50kBPS_CFG1 0x09
#define MCP_20MHz_50kBPS_CFG2 0xFA
#define MCP_20MHz_50kBPS_CFG3 0x87

#define MCP_20MHz_40kBPS_CFG1 0x09
#define MCP_20MHz_40kBPS_CFG2 0xFF
#define MCP_20MHz_40kBPS_CFG3 0x87



struct spican {
    spibus_t *bus;
};

static uint8_t
spi_read(struct spican *dev, uint8_t addr)
{
    uint8_t rx_buf[3];
    uint8_t tx_buf[3] = {INSTRUCTION_READ, addr};

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, 3, 10000);
    return rx_buf[2];
}

static void
spi_read_seq(struct spican *dev, uint8_t *buf, uint8_t addr, int len)
{
    uint8_t rx_buf[len + 2];
    uint8_t tx_buf[len + 2];

    tx_buf[0] = INSTRUCTION_READ;
    tx_buf[1] = addr;

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, len + 2, 10000);

    for (int i = 0; i < len; i++) buf[i] = rx_buf[i + 2];
}

static void
spi_write(struct spican *dev, uint8_t addr, uint8_t val)
{
    uint8_t rx_buf[3];
    uint8_t tx_buf[3] = {INSTRUCTION_WRITE, addr, val};

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, 3, 10000);
}

static void
spi_write_seq(struct spican *dev, uint8_t *buf, uint8_t addr, int len)
{
    uint8_t rx_buf[len + 2];
    uint8_t tx_buf[len + 2];

    tx_buf[0] = INSTRUCTION_WRITE;
    tx_buf[1] = addr;

    for (int i = 0; i < len; i++) tx_buf[i + 2] = buf[i];

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, len + 2, 10000);
}

static void
spi_modify(struct spican *dev, uint8_t address, uint8_t mask, uint8_t data)
{
    uint8_t rx_buf[4];
    uint8_t tx_buf[4] = {INSTRUCTION_BITMOD, address, mask, data};

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, 4, 10000);
}

static void
spi_reset(struct spican *dev)
{
    uint8_t rx_buf[1];
    uint8_t tx_buf[1] = {INSTRUCTION_RESET};

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, 1, 10000);

    uint8_t zeros[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    spi_write_seq(dev, zeros, TXB0, 14);
    spi_write_seq(dev, zeros, TXB1, 14);
    spi_write_seq(dev, zeros, TXB2, 14);
}

static uint8_t
get_status(struct spican *dev)
{
    uint8_t rx_buf[2];
    uint8_t tx_buf[2] = {INSTRUCTION_BITMOD};

    // TODO: go fasterfied. chip supportz 10MHz
    spibus_transfer(dev->bus, tx_buf, rx_buf, 2, 10000);
    return rx_buf[1];
}


void
spican_transmit(struct spican *dev, int priority, uint32_t id, uint8_t *data, uint8_t len)
{
    uint8_t base_addr;

    if ((spi_read(dev, REG_TXB0CTRL) & TXB_TXREQ) == 0) // TODO: use get_status()
        base_addr = TXB0;
    else if ((spi_read(dev, REG_TXB1CTRL) & TXB_TXREQ) == 0)
        base_addr = TXB1;
    else if ((spi_read(dev, REG_TXB2CTRL) & TXB_TXREQ) == 0)
        base_addr = TXB2;
    else
        return;

    uint8_t tx_payload[13]; // TXBUF without CTRL byte
    if (id > 0x7ff) // EXTENDED
    {
        tx_payload[EID0] = (uint8_t)(id & 0xFF);
        tx_payload[EID8] = (uint8_t)(id >> 8);
        id               = (uint16_t)(id >> 16);
        tx_payload[SIDL] = (uint8_t)(id & 0x03) | (uint8_t)((id & 0x1C) << 3) | TXB_EXIDE;
        tx_payload[SIDH] = (uint8_t)(id >> 5);
    }
    else
    {
        tx_payload[SIDH] = (uint8_t)(id >> 3);
        tx_payload[SIDL] = (uint8_t)((id & 0x07) << 5);
        tx_payload[EID0] = 0;
        tx_payload[EID8] = 0;
    }

    len &= 0xF;
    tx_payload[DLC] = len;

    for (int i = 0; i < len; i++) tx_payload[DATA + i] = data[i];

    spi_write_seq(dev, tx_payload, base_addr + 1, 13);
    printf("Error Count: %i\n", spi_read(dev, REG_EFLG));
    // spi_modify(dev, base_addr, TXB_TXREQ, TXB_TXREQ);
    spi_write(dev, base_addr, TXB_TXREQ);
}

static int
spican_read(struct spican *dev, uint8_t *addr, uint8_t *len, uint8_t *data)
{
    uint8_t stat = get_status(dev);

    uint8_t base_addr;
    uint8_t recv_int_flag;

    if (stat & STAT_RX0IF)
    {
        base_addr     = 0x61;
        recv_int_flag = CANINTF_RX0IF;
    }
    else if (stat & STAT_RX1IF)
    {
        base_addr     = 0x71;
        recv_int_flag = CANINTF_RX1IF;
    }
    else
    {
        return 0;
    }

    uint8_t tbufdata[5];

    spi_read_seq(dev, tbufdata, base_addr, 5);

    uint32_t id = (tbufdata[SIDH] << 3) + (tbufdata[SIDL] >> 5);

    if ((tbufdata[SIDL] & TXB_EXIDE) == TXB_EXIDE)
    {
        id = (id << 2) + (tbufdata[SIDL] & 0x03);
        id = (id << 8) + tbufdata[EID8];
        id = (id << 8) + tbufdata[EID0];
    }

    uint8_t dlc = (tbufdata[DLC] & DLC_MASK);
    if (dlc > 8) return 0;

    *addr = id;
    *len  = dlc;

    spi_read_seq(dev, data, base_addr + DATA, dlc);
    spi_modify(dev, REG_CANINTF, recv_int_flag, 0);

    return 1;
}

static void
set_baud(struct spican *dev, enum SPICAN_CLOCK clock, enum SPICAN_SPEED baud)
{
    uint8_t set, cfg1, cfg2, cfg3;
    set = 1;
    switch (clock)
    {
    case (MCP_8MHZ):
        switch (baud)
        {
        case (CAN_5KBPS): //   5KBPS
            cfg1 = MCP_8MHz_5kBPS_CFG1;
            cfg2 = MCP_8MHz_5kBPS_CFG2;
            cfg3 = MCP_8MHz_5kBPS_CFG3;
            break;

        case (CAN_10KBPS): //  10KBPS
            cfg1 = MCP_8MHz_10kBPS_CFG1;
            cfg2 = MCP_8MHz_10kBPS_CFG2;
            cfg3 = MCP_8MHz_10kBPS_CFG3;
            break;

        case (CAN_20KBPS): //  20KBPS
            cfg1 = MCP_8MHz_20kBPS_CFG1;
            cfg2 = MCP_8MHz_20kBPS_CFG2;
            cfg3 = MCP_8MHz_20kBPS_CFG3;
            break;

        case (CAN_31K25BPS): //  31.25KBPS
            cfg1 = MCP_8MHz_31k25BPS_CFG1;
            cfg2 = MCP_8MHz_31k25BPS_CFG2;
            cfg3 = MCP_8MHz_31k25BPS_CFG3;
            break;

        case (CAN_33KBPS): //  33.33KBPS
            cfg1 = MCP_8MHz_33k3BPS_CFG1;
            cfg2 = MCP_8MHz_33k3BPS_CFG2;
            cfg3 = MCP_8MHz_33k3BPS_CFG3;
            break;

        case (CAN_40KBPS): //  40Kbps
            cfg1 = MCP_8MHz_40kBPS_CFG1;
            cfg2 = MCP_8MHz_40kBPS_CFG2;
            cfg3 = MCP_8MHz_40kBPS_CFG3;
            break;

        case (CAN_50KBPS): //  50Kbps
            cfg1 = MCP_8MHz_50kBPS_CFG1;
            cfg2 = MCP_8MHz_50kBPS_CFG2;
            cfg3 = MCP_8MHz_50kBPS_CFG3;
            break;

        case (CAN_80KBPS): //  80Kbps
            cfg1 = MCP_8MHz_80kBPS_CFG1;
            cfg2 = MCP_8MHz_80kBPS_CFG2;
            cfg3 = MCP_8MHz_80kBPS_CFG3;
            break;

        case (CAN_100KBPS): // 100Kbps
            cfg1 = MCP_8MHz_100kBPS_CFG1;
            cfg2 = MCP_8MHz_100kBPS_CFG2;
            cfg3 = MCP_8MHz_100kBPS_CFG3;
            break;

        case (CAN_125KBPS): // 125Kbps
            cfg1 = MCP_8MHz_125kBPS_CFG1;
            cfg2 = MCP_8MHz_125kBPS_CFG2;
            cfg3 = MCP_8MHz_125kBPS_CFG3;
            break;

        case (CAN_200KBPS): // 200Kbps
            cfg1 = MCP_8MHz_200kBPS_CFG1;
            cfg2 = MCP_8MHz_200kBPS_CFG2;
            cfg3 = MCP_8MHz_200kBPS_CFG3;
            break;

        case (CAN_250KBPS): // 250Kbps
            cfg1 = MCP_8MHz_250kBPS_CFG1;
            cfg2 = MCP_8MHz_250kBPS_CFG2;
            cfg3 = MCP_8MHz_250kBPS_CFG3;
            break;

        case (CAN_500KBPS): // 500Kbps
            cfg1 = MCP_8MHz_500kBPS_CFG1;
            cfg2 = MCP_8MHz_500kBPS_CFG2;
            cfg3 = MCP_8MHz_500kBPS_CFG3;
            break;

        case (CAN_1000KBPS): //   1Mbps
            cfg1 = MCP_8MHz_1000kBPS_CFG1;
            cfg2 = MCP_8MHz_1000kBPS_CFG2;
            cfg3 = MCP_8MHz_1000kBPS_CFG3;
            break;

        default: set = 0; break;
        }
        break;

    case (MCP_16MHZ):
        switch (baud)
        {
        case (CAN_5KBPS): //   5Kbps
            cfg1 = MCP_16MHz_5kBPS_CFG1;
            cfg2 = MCP_16MHz_5kBPS_CFG2;
            cfg3 = MCP_16MHz_5kBPS_CFG3;
            break;

        case (CAN_10KBPS): //  10Kbps
            cfg1 = MCP_16MHz_10kBPS_CFG1;
            cfg2 = MCP_16MHz_10kBPS_CFG2;
            cfg3 = MCP_16MHz_10kBPS_CFG3;
            break;

        case (CAN_20KBPS): //  20Kbps
            cfg1 = MCP_16MHz_20kBPS_CFG1;
            cfg2 = MCP_16MHz_20kBPS_CFG2;
            cfg3 = MCP_16MHz_20kBPS_CFG3;
            break;

        case (CAN_33KBPS): //  20Kbps
            cfg1 = MCP_16MHz_33k3BPS_CFG1;
            cfg2 = MCP_16MHz_33k3BPS_CFG2;
            cfg3 = MCP_16MHz_33k3BPS_CFG3;
            break;

        case (CAN_40KBPS): //  40Kbps
            cfg1 = MCP_16MHz_40kBPS_CFG1;
            cfg2 = MCP_16MHz_40kBPS_CFG2;
            cfg3 = MCP_16MHz_40kBPS_CFG3;
            break;

        case (CAN_50KBPS): //  50Kbps
            cfg2 = MCP_16MHz_50kBPS_CFG2;
            cfg3 = MCP_16MHz_50kBPS_CFG3;
            break;

        case (CAN_80KBPS): //  80Kbps
            cfg1 = MCP_16MHz_80kBPS_CFG1;
            cfg2 = MCP_16MHz_80kBPS_CFG2;
            cfg3 = MCP_16MHz_80kBPS_CFG3;
            break;

        case (CAN_100KBPS): // 100Kbps
            cfg1 = MCP_16MHz_100kBPS_CFG1;
            cfg2 = MCP_16MHz_100kBPS_CFG2;
            cfg3 = MCP_16MHz_100kBPS_CFG3;
            break;

        case (CAN_125KBPS): // 125Kbps
            cfg1 = MCP_16MHz_125kBPS_CFG1;
            cfg2 = MCP_16MHz_125kBPS_CFG2;
            cfg3 = MCP_16MHz_125kBPS_CFG3;
            break;

        case (CAN_200KBPS): // 200Kbps
            cfg1 = MCP_16MHz_200kBPS_CFG1;
            cfg2 = MCP_16MHz_200kBPS_CFG2;
            cfg3 = MCP_16MHz_200kBPS_CFG3;
            break;

        case (CAN_250KBPS): // 250Kbps
            cfg1 = MCP_16MHz_250kBPS_CFG1;
            cfg2 = MCP_16MHz_250kBPS_CFG2;
            cfg3 = MCP_16MHz_250kBPS_CFG3;
            break;

        case (CAN_500KBPS): // 500Kbps
            cfg1 = MCP_16MHz_500kBPS_CFG1;
            cfg2 = MCP_16MHz_500kBPS_CFG2;
            cfg3 = MCP_16MHz_500kBPS_CFG3;
            break;

        case (CAN_1000KBPS): //   1Mbps
            cfg1 = MCP_16MHz_1000kBPS_CFG1;
            cfg2 = MCP_16MHz_1000kBPS_CFG2;
            cfg3 = MCP_16MHz_1000kBPS_CFG3;
            break;

        default: set = 0; break;
        }
        break;

    case (MCP_20MHZ):
        switch (baud)
        {
        case (CAN_40KBPS): //  40Kbps
            cfg1 = MCP_20MHz_40kBPS_CFG1;
            cfg2 = MCP_20MHz_40kBPS_CFG2;
            cfg3 = MCP_20MHz_40kBPS_CFG3;
            break;

        case (CAN_50KBPS): //  50Kbps
            cfg1 = MCP_20MHz_50kBPS_CFG1;
            cfg2 = MCP_20MHz_50kBPS_CFG2;
            cfg3 = MCP_20MHz_50kBPS_CFG3;
            break;

        case (CAN_80KBPS): //  80Kbps
            cfg1 = MCP_20MHz_80kBPS_CFG1;
            cfg2 = MCP_20MHz_80kBPS_CFG2;
            cfg3 = MCP_20MHz_80kBPS_CFG3;
            break;

        case (CAN_100KBPS): // 100Kbps
            cfg1 = MCP_20MHz_100kBPS_CFG1;
            cfg2 = MCP_20MHz_100kBPS_CFG2;
            cfg3 = MCP_20MHz_100kBPS_CFG3;
            break;

        case (CAN_125KBPS): // 125Kbps
            cfg1 = MCP_20MHz_125kBPS_CFG1;
            cfg2 = MCP_20MHz_125kBPS_CFG2;
            cfg3 = MCP_20MHz_125kBPS_CFG3;
            break;

        case (CAN_200KBPS): // 200Kbps
            cfg1 = MCP_20MHz_200kBPS_CFG1;
            cfg2 = MCP_20MHz_200kBPS_CFG2;
            cfg3 = MCP_20MHz_200kBPS_CFG3;
            break;

        case (CAN_250KBPS): // 250Kbps
            cfg1 = MCP_20MHz_250kBPS_CFG1;
            cfg2 = MCP_20MHz_250kBPS_CFG2;
            cfg3 = MCP_20MHz_250kBPS_CFG3;
            break;

        case (CAN_500KBPS): // 500Kbps
            cfg1 = MCP_20MHz_500kBPS_CFG1;
            cfg2 = MCP_20MHz_500kBPS_CFG2;
            cfg3 = MCP_20MHz_500kBPS_CFG3;
            break;

        case (CAN_1000KBPS): //   1Mbps
            cfg1 = MCP_20MHz_1000kBPS_CFG1;
            cfg2 = MCP_20MHz_1000kBPS_CFG2;
            cfg3 = MCP_20MHz_1000kBPS_CFG3;
            break;

        default: set = 0; break;
        }
        break;

    default: set = 0; break;
    }

    if (set)
    {
        spi_write(dev, REG_CNF1, cfg1);
        spi_write(dev, REG_CNF2, cfg2);
        spi_write(dev, REG_CNF3, cfg3);
    }
}

static void
set_mode(struct spican *dev, uint8_t mode)
{
    spi_modify(dev, REG_CANCTRL, CANCTRL_REQOP, mode);

    struct timespec begin;
    clock_gettime(CLOCK_REALTIME, &begin);

    while ((spi_read(dev, REG_CANSTAT) & CANSTAT_OPMOD) != mode)
    {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        long diff = (now.tv_sec - begin.tv_sec) * 1000000 + (now.tv_nsec - begin.tv_nsec);
        if (diff > 10000) // 10ms
            break;

        sched_yield();
    }
}

struct spican *
spican_create(spibus_t *bus, enum SPICAN_CLOCK clock, enum SPICAN_SPEED baud)
{
    struct spican *ret;

    ret      = malloc(sizeof(struct spican));
    ret->bus = bus;

    spi_reset(ret);
    set_mode(ret, CANCTRL_REQOP_CONFIG);
    set_baud(ret, clock, baud);
    set_mode(ret, CANCTRL_REQOP_NORMAL);

    return ret;
}

void
spican_destroy(struct spican *dev)
{
    free(dev);
}
