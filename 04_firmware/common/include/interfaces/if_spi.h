#ifndef IF_SPI_H
#define IF_SPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_SPI_OK   0
#define IF_SPI_ERR -1

typedef int (*if_spi_transfer_fn)(void *context,
                                  const uint8_t *tx,
                                  uint8_t *rx,
                                  size_t length);

typedef struct
{
    void *context;
    if_spi_transfer_fn transfer;
} if_spi_t;

#ifdef __cplusplus
}
#endif

#endif