#include <assert.h>

#include "comm_driver.h"

#include "logger.h"
#include "peripherals.h"
#include "fsl_usart.h"
#include "fsl_wwdt.h"

struct uart_comm_ctxt_t {
    USART_Type * base;
};

/**
 * @brief  Read from USART until a length is reached or the transmission goes IDLE
 *
 * @param base USART base peripheral
 * @param data Pointer where the data will be stored
 * @param length Length to be read and the length that has been read
 *
 * @returns Returns an error
 */
status_t USART_ReadBlockingUntilEndXfer(USART_Type * base,
                                        uint8_t * data,
                                        size_t * length) {
    uint8_t xferStarted = 0;
    uint32_t statusFlag;
    size_t count = 0;
    status_t status = kStatus_Success;

    /* check arguments */
    assert(!((NULL == base) || (NULL == data) || (NULL == length)));
    if ((NULL == base) || (NULL == data)) {
        return kStatus_InvalidArgument;
    }

    /* Check whether rxFIFO is enabled */
    if ((base->FIFOCFG & USART_FIFOCFG_ENABLERX_MASK) == 0U) {
        return kStatus_Fail;
    }
    for (; *length > 0U; (*length)--) {
        /* loop until rxFIFO have some data to read */
        while ((base->FIFOSTAT & USART_FIFOSTAT_RXNOTEMPTY_MASK) == 0U) {
            WWDT_Refresh(WWDT);
            if (xferStarted) {
                statusFlag = base->STAT;
                if (statusFlag & USART_STAT_RXIDLE_MASK) {
                    *length = count;
                    // LOG_WARN("Read timed out!"); // <- always happens
                    return status;
                }
            }
        }

        xferStarted = 1;
        /* check rxFIFO statusFlag */
        if ((base->FIFOSTAT & USART_FIFOSTAT_RXERR_MASK) != 0U) {
            base->FIFOCFG |= USART_FIFOCFG_EMPTYRX_MASK;
            base->FIFOSTAT |= USART_FIFOSTAT_RXERR_MASK;
            status = kStatus_USART_RxError;
            LOG_ERROR("UART RX error!");
            break;
        }
        /* check receive statusFlag */
        statusFlag = base->STAT;
        /* Clear all status flags */
        base->STAT |= statusFlag;
        if ((statusFlag & USART_STAT_PARITYERRINT_MASK) != 0U) {
            status = kStatus_USART_ParityError;
            LOG_ERROR("Parity ERROR!");
        }
        if ((statusFlag & USART_STAT_FRAMERRINT_MASK) != 0U) {
            status = kStatus_USART_FramingError;
            LOG_ERROR("Framing ERROR!");
        }
        if ((statusFlag & USART_STAT_RXNOISEINT_MASK) != 0U) {
            status = kStatus_USART_NoiseError;
            LOG_ERROR("Noise ERROR!");
        }

        if (kStatus_Success == status) {
            *data = (uint8_t)base->FIFORD;
            data++;
            count++;
        } else {
            break;
        }
    }
    *length = count;
    return status;
}


static int _comm_uart_init(void * drv) {
    (void)drv;
    return 0;
}

static int _comm_uart_write(void * drv,
                            uint8_t * buffer,
                            size_t len) {
    assert(!(NULL == drv || NULL == buffer));

    struct comm_driver_t * driver = (struct comm_driver_t *)drv;
    if (!driver) {
        LOG_ERROR("No driver present");
        return -1;
    }

    struct uart_comm_ctxt_t * ctxt =
        (struct uart_comm_ctxt_t *)driver->priv_data;
    if (!ctxt) {
        LOG_ERROR("No context found!");
        return -1;
    }

    status_t status = USART_WriteBlocking(ctxt->base, buffer, len);
    LOG_DEBUG("Status(0x%x) Write Count(%d)", status, len);

    return (int)len;
}

static int _comm_uart_read(void * drv,
                           uint8_t * buffer,
                           size_t * len) {
    assert(!(NULL == drv || NULL == buffer || NULL == len));

    struct comm_driver_t * driver = (struct comm_driver_t *)drv;
    if (!driver) {
        LOG_ERROR("No driver present");
        return -1;
    }

    struct uart_comm_ctxt_t * ctxt =
        (struct uart_comm_ctxt_t *)driver->priv_data;
    if (!ctxt) {
        LOG_ERROR("No context found!");
        return -1;
    }

    int err = USART_ReadBlockingUntilEndXfer(ctxt->base, buffer, len);
    if (err != kStatus_Success) {
        LOG_WARN("Read was not successful");
    }

    return err;
}

static void _comm_uart_close(void * drv) {
    (void)drv;
    return;
}

static struct uart_comm_ctxt_t _ctxt = {
    .base = UART_COMM,
};

static const struct comm_ops_t _uart_ops = {
    .init  = _comm_uart_init,
    .write = _comm_uart_write,
    .read  = _comm_uart_read,
    .close = _comm_uart_close,
};

struct comm_driver_t uart_comm = {
    .enabled   = true,
    .name      = "uart",
    .ops       = &_uart_ops,
    .priv_data = (void *)&_ctxt,
};
