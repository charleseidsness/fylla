/*
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
 

#ifndef DEBUG_H
#define DEBUG_H

#define M2STRX(a)	#a
#define M2STR(a)	M2STRX(a)
#define __LINESTR__	M2STR(__LINE__)

#define Error(args...) \
	fprintf(stderr, "ERROR: "__FILE__":"__LINESTR__"\t" args);\
	fprintf(stderr, "\n");\
	fflush(stderr)

#define ReturnErrIf(expr, args...) \
	if (expr) {\
		Error(#expr " -- " args);\
		return -1;\
	}

#define ReturnNULLIf(expr, args...) \
	if (expr) {\
		Error(#expr " -- " args);\
		return NULL;\
	}
	
#endif
