#ifndef SPIBUS_H
#define SPIBUS_H

#include <stdint.h>

typedef struct spibus spibus_t;

spibus_t *spibus_create(uint8_t spi_mode);
void      spibus_destroy(spibus_t *bus);
void      spibus_transfer(spibus_t *bus, unsigned char *tx, unsigned char *rx, int len, uint32_t speed);

#endif
