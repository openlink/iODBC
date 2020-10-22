/*
 *  inifile.c
 *
 *  $Id$
 *
 *  Configuration File Management
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2020 OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <iodbc.h>
#include <odbcinst.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifndef _MAC
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <unistd.h>
#include <ctype.h>

#include "inifile.h"
#include "misc.h"

#if !defined(WINDOWS) && !defined(WIN32) && !defined(OS2) && !defined(macintosh)
# include <pwd.h>
# define UNIX_PWD
#endif


extern BOOL ValidDSN (LPCSTR lpszDSN);

static PCFGENTRY __iodbcdm_cfg_poolalloc (PCONFIG p, u_int count);
static int __iodbcdm_cfg_parse (PCONFIG pconfig);

/*** READ MODULE ****/

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _MAC
static int
strcasecmp (const char *s1, const char *s2)
{
  int cmp;

  while (*s1)
    {
      if ((cmp = toupper (*s1) - toupper (*s2)) != 0)
	return cmp;
      s1++;
      s2++;
    }
  return (*s2) ? -1 : 0;
}
#endif

/*
 *  Initialize a configuration
 */
int
_iodbcdm_cfg_init (PCONFIG *ppconf, const char *filename, int doCreate)
{
  PCONFIG pconfig;

  *ppconf = NULL;

  if (!filename)
    return -1;

  if ((pconfig = (PCONFIG) calloc (1, sizeof (TCONFIG))) == NULL)
    return -1;

  pconfig->fileName = strdup (filename);
  if (pconfig->fileName == NULL)
    {
      _iodbcdm_cfg_done (pconfig);
      return -1;
    }

  /* If the file does not exist, try to create it */
  if (doCreate && access (pconfig->fileName, 0) == -1)
    {
      int fd;

      fd = creat (filename, 0644);
      if (fd)
	close (fd);
    }

  if (_iodbcdm_cfg_refresh (pconfig) == -1)
    {
      _iodbcdm_cfg_done (pconfig);
      return -1;
    }
  *ppconf = pconfig;

  return 0;
}


/*
 *  Free all data associated with a configuration
 */
int
_iodbcdm_cfg_done (PCONFIG pconfig)
{
  if (pconfig)
    {
      _iodbcdm_cfg_freeimage (pconfig);
      if (pconfig->fileName)
	free (pconfig->fileName);
      free (pconfig);
    }

  return 0;
}


/*
 *  Free the content specific data of a configuration
 */
int
_iodbcdm_cfg_freeimage (PCONFIG pconfig)
{
  char *saveName;
  PCFGENTRY e;
  u_int i;

  if (pconfig->image)
    free (pconfig->image);
  if (pconfig->entries)
    {
      e = pconfig->entries;
      for (i = 0; i < pconfig->numEntries; i++, e++)
	{
	  if (e->flags & CFE_MUST_FREE_SECTION)
	    free (e->section);
	  if (e->flags & CFE_MUST_FREE_ID)
	    free (e->id);
	  if (e->flags & CFE_MUST_FREE_VALUE)
	    free (e->value);
	  if (e->flags & CFE_MUST_FREE_COMMENT)
	    free (e->comment);
	}
      free (pconfig->entries);
    }

  saveName = pconfig->fileName;
  memset (pconfig, 0, sizeof (TCONFIG));
  pconfig->fileName = saveName;

  return 0;
}


/*
 *  This procedure reads an copy of the file into memory
 *  caching the content based on stat
 */
int
_iodbcdm_cfg_refresh (PCONFIG pconfig)
{
  struct stat sb;
  char *mem;
  int fd;

  if (pconfig == NULL || stat (pconfig->fileName, &sb) == -1)
    return -1;

  /*
   *  If our image is dirty, ignore all local changes
   *  and force a reread of the image, thus ignoring all mods
   */
  if (pconfig->dirty)
    _iodbcdm_cfg_freeimage (pconfig);

  /*
   *  Check to see if our incore image is still valid
   */
  if (pconfig->image && sb.st_size == pconfig->size
      && sb.st_mtime == pconfig->mtime)
    return 0;

  /*
   *  Now read the full image
   */
  if ((fd = open (pconfig->fileName, O_RDONLY | O_BINARY)) == -1)
    return -1;

  mem = (char *) malloc (sb.st_size + 1);
  if (mem == NULL || read (fd, mem, sb.st_size) != sb.st_size)
    {
      free (mem);
      close (fd);
      return -1;
    }
  mem[sb.st_size] = 0;

  close (fd);

  /*
   *  Store the new copy
   */
  _iodbcdm_cfg_freeimage (pconfig);
  pconfig->image = mem;
  pconfig->size = sb.st_size;
  pconfig->mtime = sb.st_mtime;

  if (__iodbcdm_cfg_parse (pconfig) == -1)
    {
      _iodbcdm_cfg_freeimage (pconfig);
      return -1;
    }

  return 1;
}


#define iseolchar(C) (strchr ("\n\r\x1a", C) != NULL)
#define iswhite(C) (strchr ("\f\t ", C) != NULL)


static char *
__iodbcdm_cfg_skipwhite (char *s)
{
  while (*s && iswhite (*s))
    s++;
  return s;
}


static int
__iodbcdm_cfg_getline (char **pCp, char **pLinePtr)
{
  char *start;
  char *cp = *pCp;

  while (*cp && iseolchar (*cp))
    cp++;
  start = cp;
  if (pLinePtr)
    *pLinePtr = cp;

  while (*cp && !iseolchar (*cp))
    cp++;
  if (*cp)
    {
      *cp++ = 0;
      *pCp = cp;

      while (--cp >= start && iswhite (*cp));
      cp[1] = 0;
    }
  else
    *pCp = cp;

  return *start ? 1 : 0;
}


static char *
rtrim (char *str)
{
  char *endPtr;

  if (str == NULL || *str == '\0')
    return NULL;

  for (endPtr = &str[strlen (str) - 1]; endPtr >= str && isspace (*endPtr);
      endPtr--);
  endPtr[1] = 0;
  return endPtr >= str ? endPtr : NULL;
}


/*
 *  Parse the in-memory copy of the configuration data
 */
static int
__iodbcdm_cfg_parse (PCONFIG pconfig)
{
  int isContinue, inString;
  char *imgPtr;
  char *endPtr;
  char *lp;
  char *section;
  char *id;
  char *value;
  char *comment;

  if (_iodbcdm_cfg_valid (pconfig))
    return 0;

  endPtr = pconfig->image + pconfig->size;
  for (imgPtr = pconfig->image; imgPtr < endPtr;)
    {
      if (!__iodbcdm_cfg_getline (&imgPtr, &lp))
	continue;

      section = id = value = comment = NULL;

      /*
         *  Skip leading spaces
       */
      if (iswhite (*lp))
	{
	  lp = __iodbcdm_cfg_skipwhite (lp);
	  isContinue = 1;
	}
      else
	isContinue = 0;

      /*
       *  Parse Section
       */
      if (*lp == '[')
	{
	  section = __iodbcdm_cfg_skipwhite (lp + 1);
	  if ((lp = strchr (section, ']')) == NULL)
	    continue;
	  *lp++ = 0;
	  if (rtrim (section) == NULL)
	    {
	      section = NULL;
	      continue;
	    }
	  lp = __iodbcdm_cfg_skipwhite (lp);
	}
      else if (*lp != ';' && *lp != '#')
	{
	  /* Try to parse
	   *   1. Key = Value
	   *   2. Value (iff isContinue)
	   */
	  if (!isContinue)
	    {
	      /* Parse `<Key> = ..' */
	      id = lp;
	      if ((lp = strchr (id, '=')) == NULL)
		continue;
	      *lp++ = 0;
	      rtrim (id);
	      lp = __iodbcdm_cfg_skipwhite (lp);
	    }

	  /* Parse value */
	  inString = 0;
	  value = lp;
	  while (*lp)
	    {
	      if (inString)
		{
		  if (*lp == inString)
		    inString = 0;
		}
	      else if (*lp == '"' || *lp == '\'')
		inString = *lp;
	      else if ((*lp == ';' || *lp == '#') && iswhite (lp[-1]))
		{
		  *lp = 0;
		  comment = lp + 1;
		  rtrim (value);
		  break;
		}
	      lp++;
	    }
	}

      /*
       *  Parse Comment
       */
      if (*lp == ';' || *lp == '#')
	comment = lp + 1;

      if (_iodbcdm_cfg_storeentry (pconfig, section, id, value, comment,
	      0) == -1)
	{
	  pconfig->dirty = 1;
	  return -1;
	}
    }

  pconfig->flags |= CFG_VALID;

  return 0;
}


int
_iodbcdm_cfg_storeentry (
    PCONFIG pconfig,
    char *section,
    char *id,
    char *value,
    char *comment,
    int dynamic)
{
  PCFGENTRY data;

  if ((data = __iodbcdm_cfg_poolalloc (pconfig, 1)) == NULL)
    return -1;

  data->flags = 0;
  if (dynamic)
    {
      if (section)
	section = strdup (section);
      if (id)
	id = strdup (id);
      if (value)
	value = strdup (value);
      if (comment)
	comment = strdup (value);

      if (section)
	data->flags |= CFE_MUST_FREE_SECTION;
      if (id)
	data->flags |= CFE_MUST_FREE_ID;
      if (value)
	data->flags |= CFE_MUST_FREE_VALUE;
      if (comment)
	data->flags |= CFE_MUST_FREE_COMMENT;
    }

  data->section = section;
  data->id = id;
  data->value = value;
  data->comment = comment;

  return 0;
}


static PCFGENTRY
__iodbcdm_cfg_poolalloc (PCONFIG p, u_int count)
{
  PCFGENTRY newBase;
  u_int newMax;

  if (p->numEntries + count > p->maxEntries)
    {
      newMax =
	  p->maxEntries ? count + p->maxEntries + p->maxEntries / 2 : count +
	  4096 / sizeof (TCFGENTRY);
      newBase = (PCFGENTRY) calloc (newMax, sizeof (TCFGENTRY));
      if (newBase == NULL)
	return NULL;
      if (p->entries)
	{
	  memcpy (newBase, p->entries, p->numEntries * sizeof (TCFGENTRY));
	  free (p->entries);
	}
      p->entries = newBase;
      p->maxEntries = newMax;
    }

  newBase = &p->entries[p->numEntries];
  p->numEntries += count;

  return newBase;
}


/*** COMPATIBILITY LAYER ***/


int
_iodbcdm_cfg_rewind (PCONFIG pconfig)
{
  if (!_iodbcdm_cfg_valid (pconfig))
    return -1;

  pconfig->flags = CFG_VALID;
  pconfig->cursor = 0;

  return 0;
}


/*
 *  returns:
 *	 0 success
 *	-1 no next entry
 *
 *	section	id	value	flags		meaning
 *	!0	0	!0	SECTION		[value]
 *	!0	!0	!0	DEFINE		id = value|id="value"|id='value'
 *	!0	0	!0	0		value
 *	0	0	0	EOF		end of file encountered
 */
int
_iodbcdm_cfg_nextentry (PCONFIG pconfig)
{
  PCFGENTRY e;

  if (!_iodbcdm_cfg_valid (pconfig) || _iodbcdm_cfg_eof (pconfig))
    return -1;

  pconfig->flags &= ~(CFG_TYPEMASK);
  pconfig->id = pconfig->value = NULL;

  while (1)
    {
      if (pconfig->cursor >= pconfig->numEntries)
	{
	  pconfig->flags |= CFG_EOF;
	  return -1;
	}
      e = &pconfig->entries[pconfig->cursor++];

      if (e->section)
	{
	  pconfig->section = e->section;
	  pconfig->flags |= CFG_SECTION;
	  return 0;
	}
      if (e->value)
	{
	  pconfig->value = e->value;
	  if (e->id)
	    {
	      pconfig->id = e->id;
	      pconfig->flags |= CFG_DEFINE;
	    }
	  else
	    pconfig->flags |= CFG_CONTINUE;
	  return 0;
	}
    }
}


int
_iodbcdm_cfg_find (PCONFIG pconfig, char *section, char *id)
{
  int atsection;

  if (!_iodbcdm_cfg_valid (pconfig) || _iodbcdm_cfg_rewind (pconfig))
    return -1;

  atsection = 0;
  while (_iodbcdm_cfg_nextentry (pconfig) == 0)
    {
      if (atsection)
	{
	  if (_iodbcdm_cfg_section (pconfig))
	    return -1;
	  else if (_iodbcdm_cfg_define (pconfig))
	    {
	      char *szId = _iodbcdm_remove_quotes (pconfig->id);
	      int bSame;
	      if (szId)
		{
		  bSame = !strcasecmp (szId, id);
		  free (szId);
		  if (bSame)
		    return 0;
		}
	    }
	}
      else if (_iodbcdm_cfg_section (pconfig)
	  && !strcasecmp (pconfig->section, section))
	{
	  if (id == NULL)
	    return 0;
	  atsection = 1;
	}
    }
  return -1;
}


/*** WRITE MODULE ****/


/*
 *  Change the configuration
 *
 *  section id    value		action
 *  --------------------------------------------------------------------------
 *   value  value value		update '<entry>=<string>' in section <section>
 *   value  value NULL		delete '<entry>' from section <section>
 *   value  NULL  NULL		delete section <section>
 */
int
_iodbcdm_cfg_write (
    PCONFIG pconfig,
    char *section,
    char *id,
    char *value)
{
  PCFGENTRY e, e2, eSect;
  int idx;
  int i;

  if (!_iodbcdm_cfg_valid (pconfig) || section == NULL)
    return -1;

  /* find the section */
  e = pconfig->entries;
  i = pconfig->numEntries;
  eSect = 0;
  while (i--)
    {
      if (e->section && !strcasecmp (e->section, section))
	{
	  eSect = e;
	  break;
	}
      e++;
    }

  /* did we find the section? */
  if (!eSect)
    {
      /* check for delete operation on a nonexisting section */
      if (!id || !value)
	return 0;

      /* add section first */
      if (_iodbcdm_cfg_storeentry (pconfig, section, NULL, NULL, NULL,
	      1) == -1
	  || _iodbcdm_cfg_storeentry (pconfig, NULL, id, value, NULL,
	      1) == -1)
	return -1;

      pconfig->dirty = 1;
      return 0;
    }

  /* ok - we have found the section - let's see what we need to do */

  if (id)
    {
      if (value)
	{
	  /* add / update a key */
	  while (i--)
	    {
	      e++;
	      /* break on next section */
	      if (e->section)
		{
		  /* insert new entry before e */
		  idx = e - pconfig->entries;
		  if (__iodbcdm_cfg_poolalloc (pconfig, 1) == NULL)
		    return -1;
		  memmove (e + 1, e,
		      (pconfig->numEntries - idx) * sizeof (TCFGENTRY));
		  e->section = NULL;
		  e->id = strdup (id);
		  e->value = strdup (value);
		  e->comment = NULL;
		  if (e->id == NULL || e->value == NULL)
		    return -1;
		  e->flags |= CFE_MUST_FREE_ID | CFE_MUST_FREE_VALUE;
		  pconfig->dirty = 1;
		  return 0;
		}

	      if (e->id && !strcasecmp (e->id, id))
		{
		  /* found key - do update */
		  if (e->value && (e->flags & CFE_MUST_FREE_VALUE))
		    {
		      e->flags &= ~CFE_MUST_FREE_VALUE;
		      free (e->value);
		    }
		  pconfig->dirty = 1;
		  if ((e->value = strdup (value)) == NULL)
		    return -1;
		  e->flags |= CFE_MUST_FREE_VALUE;
		  return 0;
		}
	    }

	  /* last section in file - add new entry */
	  if (_iodbcdm_cfg_storeentry (pconfig, NULL, id, value, NULL,
		  1) == -1)
	    return -1;
	  pconfig->dirty = 1;
	  return 0;
	}
      else
	{
	  /* delete a key */
	  while (i--)
	    {
	      e++;
	      /* break on next section */
	      if (e->section)
		return 0;	/* not found */

	      if (e->id && !strcasecmp (e->id, id))
		{
		  /* found key - do delete */
		  eSect = e;
		  e++;
		  goto doDelete;
		}
	    }
	  /* key not found - that' ok */
	  return 0;
	}
    }
  else
    {
      /* delete entire section */

      /* find e : next section */
      while (i--)
	{
	  e++;
	  /* break on next section */
	  if (e->section)
	    break;
	}
      if (i < 0)
	e++;

      /* move up e while comment */
      e2 = e - 1;
      while (e2->comment && !e2->section && !e2->id && !e2->value
	  && (iswhite (e2->comment[0]) || e2->comment[0] == ';'))
	e2--;
      e = e2 + 1;

    doDelete:
      /* move up eSect while comment */
      e2 = eSect - 1;
      while (e2->comment && !e2->section && !e2->id && !e2->value
	  && (iswhite (e2->comment[0]) || e2->comment[0] == ';'))
	e2--;
      eSect = e2 + 1;

      /* delete everything between eSect .. e */
      for (e2 = eSect; e2 < e; e2++)
	{
	  if (e2->flags & CFE_MUST_FREE_SECTION)
	    free (e2->section);
	  if (e2->flags & CFE_MUST_FREE_ID)
	    free (e2->id);
	  if (e2->flags & CFE_MUST_FREE_VALUE)
	    free (e2->value);
	  if (e2->flags & CFE_MUST_FREE_COMMENT)
	    free (e2->comment);
	}
      idx = e - pconfig->entries;
      memmove (eSect, e, (pconfig->numEntries - idx) * sizeof (TCFGENTRY));
      pconfig->numEntries -= e - eSect;
      pconfig->dirty = 1;
    }

  return 0;
}


/*
 *  Write a formatted copy of the configuration to a file
 *
 *  This assumes that the inifile has already been parsed
 */
static void
__iodbcdm_cfg_outputformatted (PCONFIG pconfig, FILE *fd)
{
  PCFGENTRY e = pconfig->entries;
  int i = pconfig->numEntries;
  int m = 0;
  int j, l;
  int skip = 0;

  while (i--)
    {
      if (e->section)
	{
	  /* Add extra line before section, unless comment block found */
	  if (skip)
	    fprintf (fd, "\n");
	  fprintf (fd, "[%s]", e->section);
	  if (e->comment)
	    fprintf (fd, "\t;%s", e->comment);

	  /* Calculate m, which is the length of the longest key */
	  m = 0;
	  for (j = 1; j <= i; j++)
	    {
	      if (e[j].section)
		break;
	      if (e[j].id && (l = strlen (e[j].id)) > m)
		m = l;
	    }

	  /* Add an extra lf next time around */
	  skip = 1;
	}
      /*
       *  Key = value
       */
      else if (e->id && e->value)
	{
	  if (m)
	    fprintf (fd, "%-*.*s = %s", m, m, e->id, e->value);
	  else
	    fprintf (fd, "%s = %s", e->id, e->value);
	  if (e->comment)
	    fprintf (fd, "\t;%s", e->comment);
	}
      /*
       *  Value only (continuation)
       */
      else if (e->value)
	{
	  fprintf (fd, "  %s", e->value);
	  if (e->comment)
	    fprintf (fd, "\t;%s", e->comment);
	}
      /*
       *  Comment only - check if we need an extra lf
       *
       *  1. Comment before section gets an extra blank line before
       *     the comment starts.
       *
       *          previousEntry = value
       *          <<< INSERT BLANK LINE HERE >>>
       *          ; Comment Block
       *          ; Sticks to section below
       *          [new section]
       *
       *  2. Exception on 1. for commented out definitions:
       *     (Immediate nonwhitespace after ;)
       *          [some section]
       *          v1 = 1
       *          ;v2 = 2   << NO EXTRA LINE >>
       *          v3 = 3
       *
       *  3. Exception on 2. for ;; which certainly is a section comment
       *          [some section]
       *          definitions
       *          <<< INSERT BLANK LINE HERE >>>
       *          ;; block comment
       *          [new section]
       */
      else if (e->comment)
	{
	  if (skip && (iswhite (e->comment[0]) || e->comment[0] == ';'))
	    {
	      for (j = 1; j <= i; j++)
		{
		  if (e[j].section)
		    {
		      fprintf (fd, "\n");
		      skip = 0;
		      break;
		    }
		  if (e[j].id || e[j].value)
		    break;
		}
	    }
	  fprintf (fd, ";%s", e->comment);
	}
      fprintf (fd, "\n");
      e++;
    }
}


/*
 *  Write the changed file back
 */
int
_iodbcdm_cfg_commit (PCONFIG pconfig)
{
  FILE *fp;

  if (!_iodbcdm_cfg_valid (pconfig))
    return -1;

  if (pconfig->dirty)
    {
      if ((fp = fopen (pconfig->fileName, "w")) == NULL)
	return -1;

      __iodbcdm_cfg_outputformatted (pconfig, fp);

      fclose (fp);

      pconfig->dirty = 0;
    }

  return 0;
}


int
_iodbcdm_cfg_next_section(PCONFIG pconfig)
{
  do
    if (0 != _iodbcdm_cfg_nextentry (pconfig))
      return -1;
  while (!_iodbcdm_cfg_section (pconfig));

  return 0;
}


#if defined(__APPLE__)

#define DSN_LST			"ODBC Data Sources"
#define DRV_LST			"ODBC Drivers"

typedef struct stat stat_t;

/**
 * return name ini file 
 *    for $HOME/.odbc.ini or ~/.odbcinst.ini file
 * OR  
 *    for $HOME/Library/ODBC/odbc.ini or $HOME/Library/ODBC/odbcinst.ini 
 *
 **/
static char *
_getinifilename (char *buf, int size, int bIsInst, int bHome)
{
  int j;
  char *ptr;

  j = STRLEN (bIsInst ? "/odbcinst.ini" : "/odbc.ini") + 1;

  if (size < j)
    return NULL;

  if (wSystemDSN == USERDSN_ONLY)
    {
      /*
       *  2b. Check either $HOME/.odbc.ini or ~/.odbc.ini
       */
      if ((ptr = getenv ("HOME")) == NULL)
	{
	  ptr = (char *) getpwuid (getuid ());

	  if (ptr != NULL)
	    ptr = ((struct passwd *) ptr)->pw_dir;
	}

      if (ptr != NULL)
        {
          if (bHome)
	    snprintf (buf, size, bIsInst ? "%s/.odbcinst.ini" : "%s/.odbc.ini",
	      ptr);
	  else
	    snprintf (buf, size,
	      bIsInst ? "%s" ODBCINST_INI_APP : "%s" ODBC_INI_APP, ptr);

          return buf;
        }
    }
  return NULL;
}



static int
_fix_home_odbc(PCONFIG pconf, char *lib_odbcini, int bIsInst)
{
  char pathbuf[1024];
  char *home_odbcini = _getinifilename (pathbuf, sizeof (pathbuf), bIsInst, TRUE);

  if (home_odbcini && lib_odbcini)
    {
      PCONFIG pCfg = NULL;
      stat_t home_stat;
      stat_t lib_stat;

      if (access(home_odbcini, R_OK)!=0)
        {
          symlink(lib_odbcini, home_odbcini);
          return 0;
        }
        else
        {
          char buf[4096];
          int rc;

          if (stat(home_odbcini, &home_stat))
            return -1;

          /* 
           * if $HOME/.odbc[inst].ini is link 
           *  to $HOME/Library/ODBC/odbc[inst].ini 
           */
          if ((home_stat.st_mode & S_IFLNK) 
              && (rc = readlink(home_odbcini, buf, sizeof(buf)))>0)
            {
              buf[rc]=0; 
              if (strcmp(buf,lib_odbcini)==0)
                return 0; /* OK  $HOME/.odbc.ini is linked to odbc.ini */
            }

          if (stat(lib_odbcini, &lib_stat))
            return -1;

          /* if $HOME/Library/ODBC/odbc[inst].ini is link 
           *  to $HOME/.odbc[inst].ini 
           */
          if ((lib_stat.st_mode & S_IFLNK) 
              && (rc = readlink(lib_odbcini, buf, sizeof(buf)))>0)
            { 
              buf[rc]=0; 
              if (strcmp(buf,home_odbcini)==0)
                return 0; /* OK  $HOME/.odbc.ini is linked to odbc.ini */
            }
        }

      /* 
       * import $HOME/.odbc[inst].ini and replace it with link 
       *  to $HOME/Library/ODBC/odbc[inst].ini 
       */
      if (!_iodbcdm_cfg_init (&pCfg, home_odbcini, FALSE))
        {
          int len = 0;
          char root_buf[4096] = {0};
          char *proot;
          char *root_val = NULL;
          int was_error = FALSE;
          char *root_lst = bIsInst?DRV_LST:DSN_LST;

          /* Move DSN/Driver list*/
          len = _iodbcdm_list_entries (pCfg, root_lst, root_buf, sizeof(root_buf));
          if (len >0)
            {
              /* move [ODBC Data Sources] section */
              for(proot = root_buf; *proot; proot += STRLEN(proot) + 1)
                {
                  if (!_iodbcdm_cfg_find (pCfg, root_lst, proot))
                    root_val = pCfg->value;
                  else
                    root_val = NULL;

                  if (!proot || !root_val) 
                    continue;

                  if (_iodbcdm_cfg_write (pconf, root_lst, proot, root_val)
                      || _iodbcdm_cfg_commit (pconf))
      		    {
      		      was_error = TRUE;
                      break;
                    }
                }

              /* move DSN defines */
              for(proot = root_buf; *proot; proot += STRLEN(proot) + 1)
                {
                  char buffer[4096];
                  char *pattr;
                  char *pattr_val;

                  len = _iodbcdm_list_entries (pCfg, proot, buffer, sizeof(buffer));
                  if (len>0)
                    {
                      /* move [DSN] description */
                      for(pattr = buffer; *pattr; pattr += STRLEN(pattr) + 1)
                        {
                          if (!_iodbcdm_cfg_find (pCfg, proot, pattr))
                            pattr_val = pCfg->value;
                          else
                            pattr_val = NULL;
                      

                          if (!pattr || !pattr_val)
                            continue;
                          
                          if (_iodbcdm_cfg_write (pconf, proot, pattr, pattr_val)
                              || _iodbcdm_cfg_commit (pconf))
      			    {
      			      was_error = TRUE;
                              break;
                            }
                        }
                    }
                }
            }
          if (was_error)
            return -1;
        }
     /* make link */
      if (access(home_odbcini, R_OK)==0)
        {
          if (unlink(home_odbcini))
            return -1;
        }

      if (symlink(lib_odbcini, home_odbcini))
        return -1;
    }
  return 0;
}


static void
_fix_office_access(char *fodbcini, int bIsInst)
{
  char *ptr;
  char *office_pwd = {"/Library/Containers/com.microsoft.Excel/Data"};

  if ((ptr = getenv ("HOME")) == NULL)
    {
      ptr = (char *) getpwuid (getuid ());

      if (ptr != NULL)
        ptr = ((struct passwd *) ptr)->pw_dir;
    }

  if (ptr != NULL && strcasestr(ptr, office_pwd)==NULL)
    {
      stat_t src_stat;
      stat_t dst_stat;
      char src[1024];
      char dst[1024];
      int j;

      snprintf (dst, sizeof(dst),
	      bIsInst ? "%s%s" ODBCINST_INI_APP 
	              : "%s%s" ODBC_INI_APP, ptr, office_pwd);

      j = STRLEN (bIsInst ? "/odbcinst.ini" : "/odbc.ini");
      dst[STRLEN(dst)-j] = 0;

      mkdir(dst, 0755);

      /* source ini file */
      snprintf (src, sizeof(src),
	      bIsInst ? "%s" ODBCINST_INI_APP : "%s" ODBC_INI_APP, ptr);
      
      /* destination ini file */
      snprintf (dst, sizeof(dst),
              bIsInst ? "%s%s" ODBCINST_INI_APP
                      : "%s%s" ODBC_INI_APP, ptr, office_pwd);

      if (access(src, R_OK)==0 && stat(src, &src_stat)==0) 
        {
          if (access(dst, R_OK)==0 && stat(dst, &dst_stat)==0)
            {
              if (src_stat.st_ino == dst_stat.st_ino)
                return; /* link existed & OK */
              else
                unlink(dst);
            }
          link(src, dst); /* create hardlink for use with MSOffice */
        }
    }
}
#endif


int
_iodbcdm_cfg_search_init(PCONFIG *ppconf, const char *filename, int doCreate)
{
  char pathbuf[1024];
  int rc;

  if (strstr (filename, "odbc.ini") || strstr (filename, "ODBC.INI"))
    {
      char *fname_odbcini = _iodbcadm_getinifile (pathbuf,
	    sizeof (pathbuf), FALSE, doCreate);
#if defined(__APPLE__)
      if (fname_odbcini && wSystemDSN == USERDSN_ONLY)
        _fix_office_access(fname_odbcini, FALSE);
#endif
      rc = _iodbcdm_cfg_init (ppconf, fname_odbcini, doCreate);
#if defined(__APPLE__)
      if (!rc && fname_odbcini && wSystemDSN == USERDSN_ONLY) 
        {
          char buf[1024];
          char *lib_odbcini = _getinifilename (buf, sizeof (buf), FALSE, FALSE);

          /* if we try open ~/Library/ODBC/odbc.ini */
          if (lib_odbcini && strcmp(fname_odbcini, lib_odbcini)==0)
            _fix_home_odbc(*ppconf, fname_odbcini, FALSE);
        }
#endif
      return rc;
    }
  else if (strstr (filename, "odbcinst.ini")
      || strstr (filename, "ODBCINST.INI"))
    {
      char *fname_odbcinst = _iodbcadm_getinifile (pathbuf,
	    sizeof (pathbuf), TRUE, doCreate);
#if defined(__APPLE__)
      if (fname_odbcinst && wSystemDSN == USERDSN_ONLY)
        _fix_office_access(fname_odbcinst, TRUE);
#endif
      rc = _iodbcdm_cfg_init (ppconf, fname_odbcinst, doCreate);
#if defined(__APPLE__)
      if (!rc && fname_odbcinst && wSystemDSN == USERDSN_ONLY) 
        {
          char buf[1024];
          char *lib_odbcinst = _getinifilename (buf, sizeof (buf), TRUE, FALSE);

          /* if we try open ~/Library/ODBC/odbcinst.ini */
          if (lib_odbcinst && strcmp(fname_odbcinst, lib_odbcinst)==0)
            _fix_home_odbc(*ppconf, fname_odbcinst, TRUE);
        }
#endif
      return rc;
    }
  else if (doCreate || (!doCreate && access(filename, R_OK) == 0))
    return _iodbcdm_cfg_init (ppconf, filename, doCreate);
  else
    return -1;
}


int
_iodbcdm_list_sections (PCONFIG pCfg, LPSTR lpszRetBuffer, int cbRetBuffer)
{
  int curr = 0, sect_len = 0;
  lpszRetBuffer[0] = 0;

  if (0 == _iodbcdm_cfg_rewind (pCfg))
    {
      while (curr < cbRetBuffer && 0 == _iodbcdm_cfg_next_section (pCfg)
	  && pCfg->section)
	{
	  sect_len = strlen (pCfg->section) + 1;
	  sect_len =
	      sect_len > cbRetBuffer - curr ? cbRetBuffer - curr : sect_len;

	  memmove (lpszRetBuffer + curr, pCfg->section, sect_len);

	  curr += sect_len;
	}
      if (curr < cbRetBuffer)
	lpszRetBuffer[curr] = 0;
      return curr;
    }
  return 0;
}


int
_iodbcdm_list_entries (PCONFIG pCfg, LPCSTR lpszSection, LPSTR lpszRetBuffer, int cbRetBuffer)
{
  int curr = 0, sect_len = 0;
  lpszRetBuffer[0] = 0;

  if (!_iodbcdm_cfg_find (pCfg, (char*)lpszSection, NULL))
    {
      while (curr < cbRetBuffer && 0 == _iodbcdm_cfg_nextentry (pCfg)
             && pCfg->section)
	{
	  if (_iodbcdm_cfg_section (pCfg))
	    break;
	  if (_iodbcdm_cfg_define (pCfg) && pCfg->id)
	    {
	      sect_len = strlen (pCfg->id) + 1;
	      sect_len =
		  sect_len >
		  cbRetBuffer - curr ? cbRetBuffer - curr : sect_len;

	      memmove (lpszRetBuffer + curr, pCfg->id, sect_len);

	      curr += sect_len;
	    }
	}
      if (curr < cbRetBuffer)
	lpszRetBuffer[curr] = 0;
      return curr;
    }
  return 0;
}


BOOL
do_create_dsns (PCONFIG pCfg, PCONFIG pInfCfg, LPSTR szDriver, LPSTR szDSNS, LPSTR szDiz)
{
  char *szValue = strdup (szDSNS), *szCurr = szValue, *szComma;
  int hasMore = FALSE;
  BOOL retcode = FALSE;

  do
    {
      szComma = strchr (szCurr, ',');
      if (szComma)
	{
	  *szComma = 0;
	  hasMore = TRUE;
	}
      else
	hasMore = FALSE;

#ifdef WIN32
      if (_iodbcdm_cfg_write (pCfg, "ODBC 32 bit Data Sources", szCurr,
	      szDiz))
#else
      if (_iodbcdm_cfg_write (pCfg, "ODBC Data Sources", szCurr, szDiz))
#endif
	goto error;

      if (!ValidDSN (szCurr) || _iodbcdm_cfg_write (pCfg, szCurr, NULL, NULL))
	goto error;

      if (_iodbcdm_cfg_find (pInfCfg, szCurr, NULL)
	  && !_iodbcdm_cfg_write (pCfg, szCurr, NULL, NULL))
	{
	  if (_iodbcdm_cfg_write (pCfg, szCurr, "Driver", szDriver))
	    goto error;
	  while (!_iodbcdm_cfg_nextentry (pInfCfg)
	      && _iodbcdm_cfg_define (pInfCfg))
	    {
	      if (_iodbcdm_cfg_write (pCfg, szCurr, pInfCfg->id,
		      pInfCfg->value))
		goto error;
	    }
	}

      szCurr = szComma + 1;
    }
  while (hasMore);

  retcode = TRUE;

error:
  free (szValue);
  return retcode;
}


BOOL
install_from_ini (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR szInfFile, LPSTR szDriver, BOOL drivers)
{
  PCONFIG pInfCfg;
  char *szKeysSection = NULL, *szDriverFile = NULL, *szSetupFile = NULL,
      *szValue = NULL, *szId = NULL, *szComma, *szComma1;
  BOOL ret = FALSE;

  if (_iodbcdm_cfg_write (pCfg, szDriver, NULL, NULL))
    return ret;

  if (_iodbcdm_cfg_init (&pInfCfg, szInfFile, FALSE))
    return ret;

  if (_iodbcdm_cfg_find (pInfCfg,
	  drivers ? "ODBC Drivers" : "ODBC Translators", szDriver))
    goto error;

#ifdef WIN32
  if (_iodbcdm_cfg_write (pCfg,
	  drivers ? "ODBC 32 bit Drivers" : "ODBC 32 bit Translators",
	  szDriver, "Installed"))
#else
  if (_iodbcdm_cfg_write (pCfg, drivers ? "ODBC Drivers" : "ODBC Translators",
	  szDriver, "Installed"))
#endif
    goto error;

  if (_iodbcdm_cfg_find (pInfCfg, szDriver,
	  drivers ? "Driver" : "Translator"))
    goto error;

  szComma = strchr (pInfCfg->value, ',');
  szComma1 = strchr (szComma + 1, ',');
  if (!szComma || !szComma1 || szComma + 1 == szComma1)
    goto error;

  *szComma1 = 0;
  szDriverFile = strdup (szComma + 1);
  if (_iodbcdm_cfg_write (pCfg, szDriver, drivers ? "Driver" : "Translator",
	  szDriverFile))
    goto error;

  if (!_iodbcdm_cfg_find (pInfCfg, szDriver, "Setup"))
    {
      szComma = strchr (pInfCfg->value, ',');
      szComma1 = strchr (szComma + 1, ',');
      if (!szComma || !szComma1 || szComma + 1 == szComma1)
	goto error;

      *szComma1 = 0;
      szSetupFile = strdup (szComma + 1);

      if (_iodbcdm_cfg_write (pCfg, szDriver, "Setup", szSetupFile))
	goto error;
    }

  if (!_iodbcdm_cfg_find (pInfCfg, szDriver, NULL))
    {
      while (!_iodbcdm_cfg_nextentry (pInfCfg)
	  && _iodbcdm_cfg_define (pInfCfg))
	if (strcmp (pInfCfg->id, drivers ? "\"Driver\"" : "\"Translator\"")
	    && strcmp (pInfCfg->id, "\"Setup\""))
	  {
	    szComma = strchr (pInfCfg->value, ',');
	    szComma1 = strchr (szComma + 1, ',');
	    if (!szComma || !szComma1 || szComma + 1 == szComma1)
	      szValue = strdup ("");
	    else
	      {
		*szComma1 = 0;
		szValue = strdup (szComma + 1);
	      }

	    szComma = strchr (pInfCfg->id, '"');
	    szComma1 = strchr (szComma + 1, '"');
	    if (!szComma || !szComma1 || szComma + 1 == szComma1)
	      goto loop_cont;
	    else
	      {
		*szComma1 = 0;
		szId = strdup (szComma + 1);
	      }

	    if (_iodbcdm_cfg_write (pCfg, szDriver, szId, szValue))
	      goto error;

	  loop_cont:
	    if (szValue)
	      {
		free (szValue);
		szValue = NULL;
	      }
	    if (szId)
	      {
		free (szId);
		szId = NULL;
	      }
	  }
    }

  if (!drivers)
    goto quit;

  szKeysSection = (char *) calloc (strlen (szDriver) + 6, sizeof (char));
  strcpy (szKeysSection, szDriver);
  strcat (szKeysSection, "-Keys");

  if (!_iodbcdm_cfg_find (pInfCfg, szKeysSection, NULL))
    {
      while (!_iodbcdm_cfg_nextentry (pInfCfg)
	  && _iodbcdm_cfg_define (pInfCfg))
	{
	  if (strcmp (pInfCfg->id, "CreateDSN"))
	    {
	      if (_iodbcdm_cfg_write (pCfg, szDriver, pInfCfg->id,
		      pInfCfg->value))
		goto error;
	    }
	  else if (!do_create_dsns (pOdbcCfg, pCfg, szDriverFile,
		  pInfCfg->value, szDriver))
	    goto error;
	}
    }

quit:
  ret = TRUE;

error:
  if (szKeysSection)
    free (szKeysSection);
  if (szDriverFile)
    free (szDriverFile);
  if (szSetupFile)
    free (szSetupFile);
  if (szValue)
    free (szValue);
  if (szId)
    free (szId);
  _iodbcdm_cfg_done (pInfCfg);
  return ret;
}


int
install_from_string (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR lpszDriver, BOOL drivers)
{
  char *szCurr = (char *) lpszDriver, *szDiz = lpszDriver;
  char *szAssignment, *szEqual, *szValue, *szDriver = NULL;

  if (_iodbcdm_cfg_write (pCfg, lpszDriver, NULL, NULL))
    return FALSE;

#ifdef WIN32
  if (_iodbcdm_cfg_write (pCfg,
	  drivers ? "ODBC 32 bit Drivers" : "ODBC 32 bit Translators",
	  lpszDriver, "Installed"))
#else
  if (_iodbcdm_cfg_write (pCfg, drivers ? "ODBC Drivers" : "ODBC Translators",
	  lpszDriver, "Installed"))
#endif
    return FALSE;

  for (szCurr = lpszDriver + strlen (lpszDriver) + 1; *szCurr;
      szCurr += strlen (szCurr) + 1)
    {
      szAssignment = strdup (szCurr);
      szEqual = strchr (szAssignment, '=');
      szValue = szEqual + 1;

      if (szEqual)
	*szEqual = 0;
      else
	goto loop_error;

      if ((drivers && !strcmp (szAssignment, "Driver")) || (!drivers
	      && !strcmp (szAssignment, "Translator")))
	{
	  if (szDriver)
	    free (szDriver);
	  szDriver = strdup (szValue);
	}

      if (drivers)
	{
	  if (strcmp (szAssignment, "CreateDSN"))
	    {
	      if (_iodbcdm_cfg_write (pCfg, lpszDriver, szAssignment, szValue))
		goto loop_error;
	    }
	  else if (!do_create_dsns (pOdbcCfg, pCfg, szDriver, szValue, szDiz))
	    goto loop_error;
	}
      else if (_iodbcdm_cfg_write (pCfg, lpszDriver, szAssignment, szValue))
	goto loop_error;

      free (szAssignment);
      continue;

    loop_error:
      if (szDriver)
	free (szDriver);
      free (szAssignment);
      return FALSE;
    }

  if (szDriver)
    free (szDriver);
  else
    return FALSE;

  return TRUE;
}
