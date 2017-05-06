/*
 * Fylla JTAG State Controller
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
#include "jport.h"
#include "jstate.h"

#define BITS_PER_BYTE	8
#define GET_BIT(byteVector, bit)	((byteVector[bit/8] >> (bit%8)) & 0x01)
/* SET_BIT depends on all bits in byteVector being 0 initially */
#define SET_BIT(byteVector, bit, val)	byteVector[bit/8] |= val << (bit%8)

typedef enum _jtagState {
	HARD_RESET,
	TEST_LOGIC_RESET,
	RUN_TEST_IDLE,
	SELECT_DR_SCAN,
	CAPTURE_DR,
	SHIFT_DR,
	EXIT1_DR,
	PAUSE_DR,
	EXIT2_DR,
	UPDATE_DR,
	SELECT_IR_SCAN,
	CAPTURE_IR,
	SHIFT_IR,
	EXIT1_IR,
	PAUSE_IR,
	EXIT2_IR,
	UPDATE_IR,
} jtagState;

struct _jstate {
	jport *port;
	jtagState state;
};

static int jstateResetMachine(jstate *state)
{
	int tdo;
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	state->state = TEST_LOGIC_RESET;
	return 0;
}

static int jstateRunTestIdle(jstate *state)
{
	int tdo;
	
	if(state->state != TEST_LOGIC_RESET) {
		ReturnErrIf(jstateResetMachine(state));
	}
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	state->state = RUN_TEST_IDLE;
	
	return 0;
}

int jstateResetPort(jstate *state)
{
	ReturnErrIf(state == NULL);
	ReturnErrIf(jportReset(state->port));
	state->state = HARD_RESET;
	return 0;
}

int jstateInstructionRegister(jstate *state, unsigned long valueOut,
		unsigned long *valueIn, int length)
{
	int tdo, tdi, i;
	
	ReturnErrIf(state == NULL);
	
	if(state->state != RUN_TEST_IDLE) {
		ReturnErrIf(jstateRunTestIdle(state));
	}
	
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	state->state = SELECT_DR_SCAN;
	
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	state->state = SELECT_IR_SCAN;
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	state->state = CAPTURE_IR;
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	*valueIn = tdo;
	state->state = SHIFT_IR;
	
	for(i = 0; i < (length - 1); i++) {
		tdi = valueOut >> i;
		ReturnErrIf(jportAccess(state->port, TMS_LOW, tdi, &tdo));
		*valueIn |= tdo << (i + 1);
	}
	
	tdi = valueOut >> (length - 1);
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, tdi, &tdo));
	state->state = EXIT1_IR;
	
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	state->state = UPDATE_IR;
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	state->state = RUN_TEST_IDLE;
	
	return 0;
}

int jstateDataRegister(jstate *state, unsigned char *valueOut,
		unsigned char *valueIn, int length)
{
	int tdo, tdi, i;
	
	ReturnErrIf(state == NULL);
	
	if(state->state != RUN_TEST_IDLE) {
		ReturnErrIf(jstateRunTestIdle(state));
	}
	
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	state->state = SELECT_DR_SCAN;
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	state->state = CAPTURE_DR;
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	state->state = SHIFT_DR;
	
	for(i = 0; i < (length - 1); i++) {
		if(i%8 == 0) {
			valueIn[i/8] = 0;
		}
		tdi = GET_BIT(valueOut, i);
		ReturnErrIf(jportAccess(state->port, TMS_LOW, tdi, &tdo));
		SET_BIT(valueIn, i, tdo);
	}
	
	tdi = GET_BIT(valueOut, (length - 1));
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, tdi, &tdo));
	SET_BIT(valueIn, (length - 1), tdo);
	state->state = EXIT1_DR;
	
	ReturnErrIf(jportAccess(state->port, TMS_HIGH, TDI_HIGH, &tdo));
	state->state = UPDATE_DR;
	
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	ReturnErrIf(jportAccess(state->port, TMS_LOW, TDI_HIGH, &tdo));
	state->state = RUN_TEST_IDLE;
	
	return 0;
}

int jstateDestroy(jstate **state)
{
	ReturnErrIf(state == NULL);
	ReturnErrIf(*state == NULL);
	
	if((*state)->port != NULL) {
		ReturnErrIf(jportDestroy(&((*state)->port)));
	}
	free(*state);
	*state = NULL;
	return 0;
}

static int jstateInit(jstate *state, char *device)
{
	state->port = jportNew(device);
	ReturnErrIf(state->port == NULL);
	ReturnErrIf(jstateResetPort(state));
	return 0;
}

jstate * jstateNew(char *device)
{
	jstate *state;
	
	state = malloc(sizeof(jstate));
	if (state != NULL) {
		if(jstateInit(state, device))
			jstateDestroy(&state);
	}
	return state;
}
