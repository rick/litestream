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
 * $Id: textutils.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


/* * * * * * * * * * * * * * * * * * * *
 * textutils.c
 *
 * Various functions to manipulate text strings.
 *
 * 7-95 Version 1
 * * * * * * * * * * * * * * * * * * * */

#include "textutils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* chop */
/* Chop the end character off a string. */
/* string: string to chop. */
/* return: string. */

char *chop (char *string)
   {
   if (!string)
      return NULL;
   else
      {
      string[strlen (string) - 1] = 0;
      return string;
      }
   }

/* cut */
/* Cut the first character off a string. */
/* string: string to cut. */
/* return: string. */

char *cut (char *string)
   {
   if (!string)
      return NULL;
   else
      {
      strcpy (string, string + 1);
      return string;
      }
   }

/* stripspaces */
/* Strips LEADING and/or TRAILING spaces from a string depending on mode of
   operation. */
/* s: the string to operate on. */
/* mode: LEADING, TRAILING.  Can be binary OR'd to do both. */
/* return: s. */ 

char *stripspaces (char *s, int mode)
   {
   if (s && strlen (s))
      {
      if (mode & LEADING)
         {
         while (strlen (s) && isspace (s[0]))
            cut (s);
	 }
      if (mode & TRAILING)
         {
         while (strlen (s) && isspace (s[strlen (s) - 1]))
            chop (s);
	 }
      }
   return s;
   }
