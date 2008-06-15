/*
 * Copyright (C) 2000 Gene Kan.  All rights reserved.  genehkan@xcf.berkeley.edu
 * Copyright (C) 2002 Litestream.org: 
 *                    Rick Bradley (rick@rickbradley.com), 
 *                    Sean Jewett  (sean@rimboy.com),
 *                    Brandon D. Valentine (brandon@dvalentine.com).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. The names Gene Kan, Litestream.org, Rick Bradley, Sean Jewett, or
 *    Brandon D. Valentine may not be used to endorse products derived
 *    from this software or portions of this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENE KAN, LITESTREAM.ORG, RICK BRADLEY,
 * SEAN JEWETT, AND BRANDON D. VALENTINE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROVIDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE OR
 * PORTIONS OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * $Id: hexdump.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


/* hexdump.c */
/* create a hex dump of a file */
/* Gene Kan, 03-10-96 */

/* 03-11-96	Version 0.1 works. */

#include <stdio.h>
#include <ctype.h>

#define WIDTH 16
#define BUFSIZE 1024

void hexdump (unsigned char *buffer, int length);

void hexdump (unsigned char *buffer, int length)
   {
   int count;
   int bytesprinted;

   for (count = 0; count < length; count += bytesprinted)
      {
      bytesprinted = 0;
      while (((count + bytesprinted) < length) && (bytesprinted < WIDTH))
         {
         if (isprint (buffer[count + bytesprinted]))
            putchar (buffer[count + bytesprinted]);
         else
            putchar ('.');
         bytesprinted++;
         }
      while (bytesprinted < WIDTH)
         {
         putchar (' ');
         bytesprinted++;
         }
      printf (" | ");
      bytesprinted = 0;
      while (((count + bytesprinted) < length) && (bytesprinted < WIDTH))
         {
         printf ("%02x ", buffer[count + bytesprinted]);
         bytesprinted++;
         }
      putchar ('\n');
      }
    return;
   }
