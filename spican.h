#ifndef SPICAN_H
#define SPICAN_H

#include "spibus.h"

typedef struct spican spican_t;

enum SPICAN_CLOCK { MCP_20MHZ, MCP_16MHZ, MCP_8MHZ };

enum SPICAN_SPEED {
    CAN_5KBPS,
    CAN_10KBPS,
    CAN_20KBPS,
    CAN_31K25BPS,
    CAN_33KBPS,
    CAN_40KBPS,
    CAN_50KBPS,
    CAN_80KBPS,
    CAN_83K3BPS,
    CAN_95KBPS,
    CAN_100KBPS,
    CAN_125KBPS,
    CAN_200KBPS,
    CAN_250KBPS,
    CAN_500KBPS,
    CAN_1000KBPS
};

struct spican *spican_create(spibus_t *bus, enum SPICAN_CLOCK canClock, enum SPICAN_SPEED canSpeed);
void           spican_destroy(struct spican *dev);

void spican_transmit(struct spican *dev, int priority, uint32_t id, uint8_t *data, uint8_t len);
static int spican_read(struct spican *dev, uint8_t *addr, uint8_t *len, uint8_t *data);
#endif
