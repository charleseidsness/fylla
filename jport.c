/*
 * Fylla Hardware Abstartion Layer -- based on this design:
 * This will have to be modified if alternate H/W is used.
 *
 * +--------------------------------------------------------------------------+
 * | 25-Pin D-Sub            74HC574(x8-FF)   ^       ^  9-Pin (JTAG)         |
 * | Parallel Port           +-------------+  |       |  +--------+           |
 * | 	+----+               |          20 |--+       +--| 9 3.3V |           |
 * |	| 2  |---->10ohm>----|  2 ----> 19 |--->47ohm>---| 1 TCK  |           |
 * |	| 3  |---->10ohm>----|  3 ----> 18 |--->47ohm>---| 2 TMS  |           |
 * |	| 4  |---->10ohm>----|  4 ----> 17 |--->47ohm>---| 6 TDI  |           |
 * |	| 5  |---->10ohm>----|  5 ----> 16 |--->47ohm>---| 7 TRST |           |
 * |	| 9  |---->10ohm>----|> 11 clk     |             |        |           |
 * |	| 11 |----<10ohm<----|  12 <--- 9  |---<47ohm<---| 3 TDO  |           |
 * |	| 25 |--+         +--|  10         |          +--| 5 GND  |           |
 * |	+----+  |         |  +-------------+          |  +--------+           |
 * |	        v         V                           V                       |
 * | NOTE: Pins 1, 6, 7, 8, and 9 of the Flip-Flop should be pulled low       |
 * |	through inidividual 1k ohm resistors.                                 |
 * | NOTE: Should use 100nF and 1uF bypass caps sonnected between Vcc and GND |
 * +--------------------------------------------------------------------------+
 *
 * Copyright 2005 Cooper Street Innovations Inc.
 * Author: Charles Eidsness	<charles@cooper-street.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ppdev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "debug.h"
#include "jport.h"

#define DEV_NAME_LEN	256

#define JTAG_TCK	0
#define JTAG_TMS	1
#define JTAG_TDI	2
#define JTAG_TRST	3
#define JTAG_TDO	7
#define PORT_CLK	7

struct _jport {
	int pd;
	char dev[DEV_NAME_LEN];
};

static int jportGet(int pd, int *value)
{
	ReturnErrIf(ioctl(pd, PPRSTATUS, value));
	return 0;
}

static int jportPut(int pd, int value)
{
	/* Write data with port-clock low */
	value = (value & 0x0f) | (0 << PORT_CLK);
	ReturnErrIf(ioctl(pd, PPWDATA, &value));
	/* Drive port-clock high to latch data in port */
	value = (value & 0x0f) | (1 << PORT_CLK);
	ReturnErrIf(ioctl(pd, PPWDATA, &value));
	return 0;
}

int jportAccess(jport *port, int tms, int tdi, int *tdo)
{
	int value;
	
	ReturnErrIf(port == NULL);
	
	value = ((tms & 0x01) << JTAG_TMS) | ((tdi & 0x01) << JTAG_TDI) | 
			(1 << JTAG_TRST);
	
	/* Drive TCK low */
	ReturnErrIf(jportPut(port->pd, value | (0 << JTAG_TCK)));
	
	/* Drive TCK high */
	ReturnErrIf(jportPut(port->pd, value | (1 << JTAG_TCK)));
	
	/* Drive TCK low */
	/* ReturnErrIf(jportPut(port->pd, value | (0 << JTAG_TCK))); */
	
	/* Read TDO */
	ReturnErrIf(jportGet(port->pd, &value));
	*tdo = (~(value >> JTAG_TDO) & 0x01);
	
	/* printf("TMS: %i TDI: %i TDO: %i\n", tms, tdi, *tdo); */
	
	return 0;
}

int jportReset(jport *port)
{
	ReturnErrIf(port == NULL);
	
	/* Drive TRST low and everyting else high*/
	ReturnErrIf(jportPut(port->pd, (0 << JTAG_TRST) |  (1 << JTAG_TCK) |
		(1 << JTAG_TRST) | (1 << JTAG_TDI)));
	
	usleep(500000);
	
	/* Drive TRST high and everyting else high*/
	ReturnErrIf(jportPut(port->pd, (1 << JTAG_TRST) |  (1 << JTAG_TCK) |
		(1 << JTAG_TRST) | (1 << JTAG_TDI)));
	
	return 0;
}

int jportDestroy(jport **port)
{
	ReturnErrIf(port == NULL);
	ReturnErrIf(*port == NULL);
	
	/* Drive TRST low and everyting else high*/
	ReturnErrIf(jportPut((*port)->pd, (0 << JTAG_TRST) |  (1 << JTAG_TCK) |
		(1 << JTAG_TRST) | (1 << JTAG_TDI)));
	
	if((*port)->pd != -1) {
		ReturnErrIf((ioctl((*port)->pd, PPRELEASE) == -1));
		ReturnErrIf((close((*port)->pd) == -1));
	}
	free(*port);
	*port = NULL;
	return 0;
}

static int jportInit(jport *port, char *device)
{
	port->pd = -1;
	strncpy(port->dev, device, DEV_NAME_LEN);
	
	port->pd = open(port->dev, O_RDWR);
	ReturnErrIf(port->pd == -1);
	
	ReturnErrIf(ioctl(port->pd, PPCLAIM) == -1);
	
	return 0;
}

jport * jportNew(char *device)
{
	jport *port;
	
	port = malloc(sizeof(jport));
	if (port != NULL) {
		if(jportInit(port, device))
			jportDestroy(&port);
	}
	return port;
}
