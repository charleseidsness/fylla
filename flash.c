/*
 * Fylla FLASH Sromport; only Spasion SL29GLxxxM is sromported at this time
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
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRROMTION) HOWEVER CAUSED AND ON
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
#include "processor.h"
#include "flash.h"

struct _flash {
	processor *up;
};

int flashErase(flash *rom)
{
	int data;
	
	ReturnErrIf(rom == NULL);
	
	ReturnErrIf(processorWriteBus(rom->up, 0xaaa, 0xaa));
	ReturnErrIf(processorWriteBus(rom->up, 0x555, 0x55));
	ReturnErrIf(processorWriteBus(rom->up, 0xaaa, 0x80));
	ReturnErrIf(processorWriteBus(rom->up, 0xaaa, 0xaa));
	ReturnErrIf(processorWriteBus(rom->up, 0x555, 0x55));
	ReturnErrIf(processorWriteBus(rom->up, 0xaaa, 0x10));
	data = 0x0;
	printf("Erasing FLASH ");
	while(data != 0xffff) {
		ReturnErrIf(processorReadBus(rom->up, 0x0000, &data));
		printf(".");
		fflush(stdout);
		sleep(2);
	}
	printf("\n");
	
	return 0;
}

int flashRead(flash *rom, int address, int *data)
{
	ReturnErrIf(rom == NULL);
	ReturnErrIf(processorReadBus(rom->up, address, data));
	return 0;
}

int flashWrite(flash *rom, int address, int data)
{
	ReturnErrIf(rom == NULL);
	
	ReturnErrIf(processorWriteBus(rom->up, 0x0aaa, 0xaa));
	ReturnErrIf(processorWriteBus(rom->up, 0x0555, 0x55));
	ReturnErrIf(processorWriteBus(rom->up, 0x0aaa, 0xa0));
	ReturnErrIf(processorWriteBus(rom->up, address, data));
	
	return 0;
}

int flashDestroy(flash **rom)
{
	ReturnErrIf(rom == NULL);
	ReturnErrIf(*rom == NULL);
	
	if((*rom)->up != NULL) {
		ReturnErrIf(processorDestroy(&((*rom)->up)));
	}
	free(*rom);
	*rom = NULL;
	return 0;
}

static int flashInit(flash *rom, char *device)
{
	rom->up = processorNew(device);
	ReturnErrIf(rom->up == NULL);
	
	return 0;
}

flash * flashNew(char *device)
{
	flash *rom;
	
	rom = malloc(sizeof(flash));
	if (rom != NULL) {
		if(flashInit(rom, device)) {
			flashDestroy(&rom);
		}
	}
	return rom;
}
