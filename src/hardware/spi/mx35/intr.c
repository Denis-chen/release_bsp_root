/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */


#include "mxcspi.h"



/*
 * We use the same buffer for transmit and receive
 * For exchange, that's exactly what we wanted
 * For Read, it doesn't matter what we write to SPI, so we are OK.
 * For transmit, the receive data is put at the buffer we just transmitted, we are still OK.
 */

static const struct sigevent *spi_intr(void *area, int id)
{
	mx35_cspi_t	*dev = area;
	uintptr_t	base = dev->vbase;
	uint32_t	data;
    int			tmp, count, i = 0;

    // clear source
    out32(  base + MX35_CSPI_STATREG, CSPI_STATREG_TC);

	if (dev->burst) {
		/* how many bytes need to be transmited */
		count = dev->xlen - dev->tlen;  

		/* fill tx buffer(if required) */
		if (count > 0) {
			if (count >= MX35_CSPI_BURST_MAX) {  

				tmp = MX35_CSPI_BURST_MAX * 8 - 1;		// next SPI burst length

			} else {

				switch (tmp = count % 4) {
					case 1:   // only write 8 bits in TXFIFO
						if (1 == dev->dlen) {
							out32(base + MX35_CSPI_TXDATA, dev->pbuf[dev->tlen]);
							dev->tlen++;
						}
						break;

					case 2:   // only write 16 bits in TXFIFO
						if (1 == dev->dlen) {
							tmp = (dev->pbuf[dev->tlen] << 8) | (dev->pbuf[dev->tlen + 1]);
							out32(base + MX35_CSPI_TXDATA, tmp);
							dev->tlen += 2;

						} else if (2 == dev->dlen) {
							tmp = *(uint16_t *)(&dev->pbuf[dev->tlen]);
							out32(base + MX35_CSPI_TXDATA, tmp);
							dev->tlen += 2;
						}
						break;

					case 3:   // only write 24 bits in TXFIFO
						if (1 == dev->dlen) {
							tmp = (dev->pbuf[dev->tlen] << 16) | (dev->pbuf[dev->tlen + 1] << 8) | (dev->pbuf[dev->tlen + 2]); 
							out32(base + MX35_CSPI_TXDATA, tmp);
							dev->tlen += 3;
						}
						break;
					default:
						break;
				}

				tmp = count * 8 - 1; 						// next SPI burst length		 
			}

			tmp = (in32(base + MX35_CSPI_CONREG) & ~CSPI_CONREG_BCNT_MASK) | (tmp << CSPI_CONREG_BCNT_POS);
			out32(base + MX35_CSPI_CONREG, tmp);	

			while ((dev->xlen > dev->tlen) && (i < 8)) {	// write rest of data to TXFIFO
				switch (dev->dlen) {
					case 1:
						data = (dev->pbuf[dev->tlen] << 24) | (dev->pbuf[dev->tlen + 1] << 16) 
								| (dev->pbuf[dev->tlen + 2] << 8) | (dev->pbuf[dev->tlen + 3]); 
						break;
					case 2:
						data = (*(uint16_t *)(&dev->pbuf[dev->tlen]) << 16) 
								| (*(uint16_t *)(&dev->pbuf[dev->tlen + 2]));
						break;
					case 4:
						data = *(uint32_t *)(&dev->pbuf[dev->tlen]);
						break;
					default:
						data = 0;
						break;
				}

				out32(base + MX35_CSPI_TXDATA, data);
				dev->tlen += 4;
				i++;
			}
		}

		/* how many words received */
		count = ((in32(base + MX35_CSPI_TESTREG) & CSPI_TESTREG_RXCNT_MASK) >> CSPI_TESTREG_RXCNT);

		for(i=0; i < count ; i++) {
			data = in32(base + MX35_CSPI_RXDATA);

			switch (dev->dlen) {
				case 1:
					dev->pbuf[dev->rlen++] = data>>24;
					dev->pbuf[dev->rlen++] = data>>16;
					dev->pbuf[dev->rlen++] = data>>8;
					dev->pbuf[dev->rlen++] = data;
					break;
				case 2:
					*(uint16_t *)(&dev->pbuf[dev->rlen]) = data>>16;
					dev->rlen += 2;
					*(uint16_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 2;
					break;
				case 4:
					*(uint32_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 4;
					break;
			}
		}

	} else {
		/* how many words received */
		count = ((in32(base + MX35_CSPI_TESTREG) & CSPI_TESTREG_RXCNT_MASK) >> CSPI_TESTREG_RXCNT);

		//empty RX buffer and fill tx buffer(if required)
		for(i=0; i < count ; i++) {

			data = in32(base + MX35_CSPI_RXDATA);

			switch (dev->dlen) {
				case 1:
					dev->pbuf[dev->rlen++] = data;

					/*
					* More to transmit?
					*/
					if (dev->tlen < dev->xlen)
						out32(base + MX35_CSPI_TXDATA, dev->pbuf[dev->tlen++]);
					break;
				case 2:
					*(uint16_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 2;

					/*
					* More to transmit?
					*/
					if (dev->tlen < dev->xlen) {
						out32(base + MX35_CSPI_TXDATA, *(uint16_t *)(&dev->pbuf[dev->tlen]));
						dev->tlen += 2;
					}
					break;
				case 3:
				case 4:
					*(uint32_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 4;

					/*
					* More to transmit?
					*/
					if (dev->tlen < dev->xlen) {
						out32(base + MX35_CSPI_TXDATA, *(uint32_t *)(&dev->pbuf[dev->tlen]));
						dev->tlen += 4;
					}
					break;
			}
		}
	}

	if (dev->rlen >= dev->xlen) {
        out32(base + MX35_CSPI_INTREG, 0);	/* Disable interrupt */
        return (&dev->spievent);
    } else {
        /* Start exchange */
        out32(  base + MX35_CSPI_CONREG, 
                in32(base + MX35_CSPI_CONREG) | CSPI_CONREG_XCH );
    }

    return NULL;
 }

int mx35_attach_intr(mx35_cspi_t *mx35)
{
	if ((mx35->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1)
		return -1;

	if ((mx35->coid = ConnectAttach(0, 0, mx35->chid, _NTO_SIDE_CHANNEL, 0)) == -1) 
		goto fail0;

	mx35->spievent.sigev_notify   = SIGEV_PULSE;
	mx35->spievent.sigev_coid     = mx35->coid;
	mx35->spievent.sigev_code     = MX35_CSPI_EVENT;
	mx35->spievent.sigev_priority = MX35_CSPI_PRIORITY;

	/*
	 * Attach SPI interrupt
	 */
	mx35->iid = InterruptAttach(mx35->irq, spi_intr, mx35, 0, _NTO_INTR_FLAGS_TRK_MSK);

	if (mx35->iid != -1)
		return 0;

	ConnectDetach(mx35->coid);
fail0:
	ChannelDestroy(mx35->chid);

	return -1;
}
