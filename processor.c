/*
 * Fylla Processor Support; only AMD Au1000 is supported at this time
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

#include "debug.h"
#include "jstate.h"
#include "processor.h"
#include "processor.bsdl.h"

#define BITS_PER_BYTE	8
#define GET_BIT(byteVector, bit)	((byteVector[bit/8] >> (bit%8)) & 0x01)
#define SET_BIT_HIGH(byteVector, bit)	(byteVector[bit/8] |= (0x1 << bit%8))
#define SET_BIT_LOW(byteVector, bit)	(byteVector[bit/8] &= ~(0x1 << bit%8))
#define SET_BIT(byteVector, bit, value) \
	if((value & 0x01)) {\
		SET_BIT_HIGH(byteVector, bit);\
	} else {\
		SET_BIT_LOW(byteVector, bit);\
	}
#define SET_CNTRL_OUT(byteVector, bit)	SET_BIT_HIGH(byteVector, bit)
#define SET_CNTRL_IN(byteVector, bit)	SET_BIT_LOW(byteVector, bit)

struct _processor {
	jstate *state;
};

static int processorInitScanRegister(processor *up)
{
	unsigned long ulDummy;
	unsigned char defState[BOUNDARY_BYTES] = DEFAULT_CELLS;
	unsigned char currentState[BOUNDARY_BYTES];
	
	/* Send SAMPLE command, this sets deafult pin settings but doesn't
		enter test mode yet */
	ReturnErrIf(jstateInstructionRegister(up->state, CMD_SAMPLE, &ulDummy, 
			CMD_LENGTH));
	/* Brings devices on card, like FLASH out of reset */
	//SET_BIT_HIGH(defState, RESET_OUT_N_OUTP2);
	ReturnErrIf(jstateDataRegister(up->state, defState, currentState, 
			BOUNDARY_LENGTH));
	/* Send the EXTEST command, now device is in test mode, any writes
		to the data resgiter will change the bit states */
	ReturnErrIf(jstateInstructionRegister(up->state, CMD_EXTEST, &ulDummy, 
			CMD_LENGTH));
	
	return 0;
}

int processorReadBus(processor *up, int address, int *data)
{
	unsigned char newState[BOUNDARY_BYTES] = DEFAULT_CELLS;
	unsigned char oldState[BOUNDARY_BYTES];
	int i;
	
	SET_BIT_HIGH(newState, RESET_OUT_N_OUTP2);
	SET_CNTRL_OUT(newState, RCE_N_0_CNTRL);
	SET_BIT_HIGH(newState, RCE_N_0_OUTP3);
	SET_CNTRL_OUT(newState, ROE_N_CNTRL);
	SET_BIT_HIGH(newState, ROE_N_OUTP3);
	SET_CNTRL_OUT(newState, RWE_N_CNTRL);
	SET_BIT_HIGH(newState, RWE_N_OUTP3);
	
	/* set address bus, based on cell control pattern, exclusive to
		the au1000 */
	for(i = 0; i < 32; i++) {
		SET_CNTRL_OUT(newState, (RAD_0_CNTRL - (i*3)));
		SET_BIT(newState, (RAD_0_OUTP3 - (i*3)), (address >> i));
	}
	
	/* Drive address with control bits high */
	ReturnErrIf(jstateDataRegister(up->state, newState, oldState, 
				BOUNDARY_LENGTH));
	
	/* Drive control bits low */
	SET_BIT_LOW(newState, RCE_N_0_OUTP3);
	SET_BIT_LOW(newState, ROE_N_OUTP3);
	ReturnErrIf(jstateDataRegister(up->state, newState, oldState, 
				BOUNDARY_LENGTH));
	
	/* Drive control bits high and sample data */
	SET_BIT_HIGH(newState, RCE_N_0_OUTP3);
	SET_BIT_HIGH(newState, ROE_N_OUTP3);
	ReturnErrIf(jstateDataRegister(up->state, newState, oldState, 
				BOUNDARY_LENGTH));
	
	*data = 0;
	for(i = 0; i < 16; i++) {
		*data |= GET_BIT(oldState, (RD_0_INP - (i*3))) << i;
	}
	
	return 0;
}

int processorWriteBus(processor *up, int address, int data)
{
	unsigned char newState[BOUNDARY_BYTES] = DEFAULT_CELLS;
	unsigned char oldState[BOUNDARY_BYTES];
	int i;
	
	SET_BIT_HIGH(newState, RESET_OUT_N_OUTP2);
	SET_CNTRL_OUT(newState, RCE_N_0_CNTRL);
	SET_BIT_HIGH(newState, RCE_N_0_OUTP3);
	SET_CNTRL_OUT(newState, ROE_N_CNTRL);
	SET_BIT_HIGH(newState, ROE_N_OUTP3);
	SET_CNTRL_OUT(newState, RWE_N_CNTRL);
	SET_BIT_HIGH(newState, RWE_N_OUTP3);
	
	/* set address bus, based on cell control pattern, exclusive to
		the au1000 */
	for(i = 0; i < 32; i++) {
		SET_CNTRL_OUT(newState, (RAD_0_CNTRL - (i*3)));
		SET_BIT(newState, (RAD_0_OUTP3 - (i*3)), (address >> i));
	}
	
	/* set data on bus, based on cell control pattern, exclusive to
		the au1000 */
	for(i = 0; i < 32; i++) {
		SET_CNTRL_OUT(newState, (RD_0_CNTRL - (i*3)));
		SET_BIT(newState, (RD_0_OUTP3 - (i*3)), (data >> i));
	}
	
	/* Drive address and data with control bits high */
	/* ReturnErrIf(jstateDataRegister(up->state, newState, oldState, 
				BOUNDARY_LENGTH)); */
	
	/* Drive control bits low */
	SET_BIT_LOW(newState, RCE_N_0_OUTP3);
	SET_BIT_LOW(newState, RWE_N_OUTP3);
	ReturnErrIf(jstateDataRegister(up->state, newState, oldState, 
				BOUNDARY_LENGTH));
	
	/* Drive control bits high */
	SET_BIT_HIGH(newState, RCE_N_0_OUTP3);
	SET_BIT_HIGH(newState, RWE_N_OUTP3);
	ReturnErrIf(jstateDataRegister(up->state, newState, oldState, 
				BOUNDARY_LENGTH));
	
	return 0;
}

int processorDestroy(processor **up)
{
	ReturnErrIf(up == NULL);
	ReturnErrIf(*up == NULL);
	
	if((*up)->state != NULL) {
		ReturnErrIf(jstateDestroy(&((*up)->state)));
	}
	free(*up);
	*up = NULL;
	return 0;
}

static int processorInit(processor *up, char *device)
{
	up->state = jstateNew(device);
	ReturnErrIf(up->state == NULL);
	
	ReturnErrIf(processorInitScanRegister(up));
	
	return 0;
}

processor * processorNew(char *device)
{
	processor *up;
	
	up = malloc(sizeof(processor));
	if (up != NULL) {
		if(processorInit(up, device)) {
			processorDestroy(&up);
		}
	}
	return up;
}
