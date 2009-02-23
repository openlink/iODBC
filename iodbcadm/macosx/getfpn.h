/*
 *  getfpn.h
 *
 *  $Id$
 *
 *  Functions to get the full path name of a file
 *
 *  (C)Copyright 1999-2009 OpenLink Software.
 *  All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef __APPLE__
#  include <Carbon/Carbon.h>
#else
#  include <Folders.h>
#  include <Files.h>
#endif

#include <string.h>
#include <stdlib.h>

#ifndef _GETFPN_H
#define _GETFPN_H

char *get_full_pathname (long dirID, short volID);

#ifndef strdup
char *strdup (const char *src);
#endif

#endif
