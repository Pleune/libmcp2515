#include "spibus.h"

#include <fcntl.h>
#include <getopt.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct spibus {
    int fd;
};

struct spibus *
spibus_create(uint8_t spi_mode)
{
    int fd_;

    if ((fd_ = open("/dev/spidev1.0", O_RDWR)) < 0)
    {
        perror("Failed to open spidev.\n");
        return 0;
    }

    struct spibus *ret = malloc(sizeof(struct spibus));
    ret->fd            = fd_;

    if (ioctl(fd_, SPI_IOC_WR_MODE, &spi_mode) == -1)
    {
        perror("can't set spi mode");
        close(fd_);
        free(ret);
        return 0;
    }

    if (ioctl(fd_, SPI_IOC_RD_MODE, &spi_mode) == -1)
    {
        perror("can't get spi mode");
        close(fd_);
        free(ret);
        return 0;
    }

    return ret;
}

void
spibus_destroy(struct spibus *bus)
{
    close(bus->fd);
    free(bus);
}

void
spibus_transfer(struct spibus *bus, unsigned char *tx, unsigned char *rx, int len, uint32_t speed)
{
    struct spi_ioc_transfer tr = {
        .tx_buf        = (unsigned long)tx,
        .rx_buf        = (unsigned long)rx,
        .len           = len,
        .delay_usecs   = 0,
        .speed_hz      = speed,
        .bits_per_word = 8,
    };

    if (ioctl(bus->fd, SPI_IOC_MESSAGE(1), &tr) < 1) perror("can't send spi message");
}
