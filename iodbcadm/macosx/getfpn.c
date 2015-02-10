/*
 *  getfpn.c
 *
 *  $Id$
 *
 *  Get full path name of a file functions.
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2015 by OpenLink Software <iodbc@openlinksw.com>
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

#include <getfpn.h>
#include <sql.h>
#include <sqltypes.h>
#include <unicode.h>

#ifdef __APPLE__
#  define PATH_SEPARATOR	"/"
#else
#  define PATH_SEPARATOR	":"
#endif

char *
get_full_pathname (long dirID, short volID)
{
  FSVolumeInfoParam vol;
  HFSUniStr255 volname;
  CInfoPBRec pb;
  OSErr result;
  Str255 dir;
  char *pathname = NULL;
  char *prov = NULL;
  SInt16 vdefNum; 
  SInt32 dummy;

  pb.dirInfo.ioNamePtr = dir;
  pb.dirInfo.ioVRefNum = volID;
  pb.dirInfo.ioDrParID = dirID;
  pb.dirInfo.ioFDirIndex = -1;

  do
    {
      pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;
      result = PBGetCatInfo (&pb, false);
      if (result != noErr)
	return NULL;
      if (pb.dirInfo.ioDrDirID != fsRtDirID)
	{
	  prov = pathname;
	  pathname =
	      (char *) malloc ((pathname ? strlen (pathname) : 0) +
	      (unsigned char) dir[0] + 2);
	  strcpy (pathname, PATH_SEPARATOR);
	  strncat (pathname, (char *) dir + 1, (unsigned char) dir[0]);
	}
#ifdef macintosh
      else
	{
	  prov = pathname;
	  pathname =
	      (char *) malloc ((pathname ? strlen (pathname) : 0) +
	      (unsigned char) dir[0] + 1);
	  pathname[0] = 0;
	  strncat (pathname, (char *) dir + 1, (unsigned char) dir[0]);
	}
#endif

      if (prov)
	{
	  strcat (pathname, prov);
	  free (prov);
         prov = NULL;
	}
    }
  while (pb.dirInfo.ioDrDirID != fsRtDirID);

#ifdef __APPLE__
  /* Get also the volume name */
  FindFolder (kOnSystemDisk, kSystemFolderType, FALSE, &vdefNum, &dummy);
  if(vdefNum != volID)
    {
      vol.ioVRefNum = volID;
      vol.volumeIndex = 0;
      vol.whichInfo = kFSVolInfoNone;
      vol.volumeName = &volname;
      vol.ref = NULL;

      PBGetVolumeInfoSync(&vol);
      if(volname.length)
        {
          wchar_t volwchar[1024];
          char *volutf8 = NULL;

          /* Convert the UniString255 to wchar */
          for(dummy = 0 ; dummy < volname.length ; dummy++)
            volwchar[dummy] = volname.unicode[dummy];
          volwchar[dummy] = L'\0';

          /* Then transform it in UTF8 */
          volutf8 = dm_SQL_WtoU8 (volwchar, SQL_NTS);
          if(volutf8)
            {
              prov = pathname;
              pathname =
                (char*) malloc ((pathname ? strlen (pathname) : 0) +
                volname.length + strlen("/Volumes/") + 2);
              strcpy(pathname, "/Volumes/");
              strcat(pathname, volutf8);
              if(prov)
                {
                  strcat(pathname, prov);
                  free(prov);
                }
              free(volutf8);
            }
        }
    }
#endif

  return pathname;
}
