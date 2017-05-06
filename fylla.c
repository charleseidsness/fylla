/*
 * BRIEF MODULE DESCRIPTION
 *  Drives the jport hardware to program an AMD Au1000's FLASH via the
 *  JTAG port.
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
 * History:
 *
 * 2005-12-29 Charles Eidsness	-- Original verion.
 * 2005-01-07 Charles Eidsness	-- Removed some unessicarly toggeling to
 *		increase the download speed.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "debug.h"
#include "flash.h"

#define FYLLA_VERSION "1.1"
#define STR_LEN	256

#define ExitFailureIf(expr, args...) \
	if (expr) {\
		Error(#expr " -- " args);\
		flashDestroy(&rom);\
		exit(EXIT_FAILURE);\
	}

typedef enum {
	NONE,
	LOAD_FLASH,
	LOAD_FLASH_CHECK,
	READ_MEM,
	WRITE_MEM,
	ERASE_MEM,
} option;

int main(int argc, char *argv[])
{
	char device[STR_LEN];
	char file[STR_LEN];
	int address = 0x00400000, wData = 0x0000, rData = 0x0000;
	flash *rom = NULL;
	int opt, fd;
	option action = NONE;
	
	strcpy(device, "/dev/parport0");
	strcpy(file, "u-boot.bin");
	
	printf("Fylla Version %s \n", FYLLA_VERSION);
	printf("(c) 2005 Cooper Street Innovations\n");
	
	while((opt = getopt(argc, argv, "d:f:a:lrw:ehc")) != -1) {
		switch(opt) {
		case 'd':
			strncpy(device, optarg, STR_LEN);
			break;
		case 'f':
			strncpy(file, optarg, STR_LEN);
			break;
		case 'a':
			sscanf(optarg, "%x", &address);
			break;
		case 'l':
			action = LOAD_FLASH;
			break;
		case 'c':
			action = LOAD_FLASH_CHECK;
			break;
		case 'r':
			action = READ_MEM;
			break;
		case 'w':
			sscanf(optarg, "%x", &wData);
			action = WRITE_MEM;
			break;
		case 'e':
			action = ERASE_MEM;
			break;
		case 'h':
		default:
			printf("Usage:\n");
			printf("\t-d <device>\t-> default is /dev/parport0\n");
			printf("\t-f <file>\t-> default is u-boot.bin\n");
			printf("\t-a <address>\t->  default is 0x00400000\n");
			printf("\t-l\t\t-> load flash\n");
			printf("\t-c\t\t-> load flash, check values\n");
			printf("\t-r\t\t-> read from address\n");
			printf("\t-w <value>\t-> write value to address \n");
			printf("\t-e\t\t-> erase flash\n\n");
			exit(EXIT_SUCCESS);
			break;
		}
	}
	
	
	printf("Fylla Settings\n");
	printf("Device: %s\n", device);
	printf("File: %s\n", file);
	printf("Address: 0x%08X\n", address);
	
	rom = flashNew(device);
	ExitFailureIf(rom == NULL);
	
	switch(action) {
		case NONE:
			printf("\nTry fylla -h for help\n\n");
			break;
		case LOAD_FLASH:
			fd = open(file, O_RDONLY);
			ExitFailureIf(fd < 0);
			ExitFailureIf(flashErase(rom));
			printf("Writing data, This will take a while \n");
			fflush(stdout);
			while(read(fd, &wData, 2) == 2) {
				ExitFailureIf(flashWrite(rom, address, wData));
				address = address + 2;
				if(address%128 == 0) {
						printf("#");
						fflush(stdout);
				}
			}
			printf("\n");
			close(fd);
			break;
		case LOAD_FLASH_CHECK:
			fd = open(file, O_RDONLY);
			ExitFailureIf(fd < 0);
			ExitFailureIf(flashErase(rom));
			printf("Writing data, This will take a while \n");
			fflush(stdout);
			while(read(fd, &wData, 2) == 2) {
				ExitFailureIf(flashWrite(rom, address, wData));
				ExitFailureIf(flashRead(rom, address, &rData));
				ExitFailureIf(wData != rData);
				address = address + 2;
				if(address%128 == 0) {
						printf("#");
						fflush(stdout);
				}
			}
			printf("\n");
			close(fd);
			break;
		case READ_MEM:
			ExitFailureIf(flashRead(rom, address, &rData));
			printf("0x%08X = 0x%04X\n", address, rData);
			break;
		case WRITE_MEM:
			ExitFailureIf(flashWrite(rom, address, wData));
			ExitFailureIf(flashRead(rom, address, &rData));
			ExitFailureIf(wData != rData);
			printf("0x%08X = 0x%04X\n", address, rData);
			break;
		case ERASE_MEM:
			ExitFailureIf(flashErase(rom));
			break;
	}
	
	ExitFailureIf(flashDestroy(&rom));
	
	exit(EXIT_SUCCESS);
}
