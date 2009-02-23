/*
 *  getfpn.c
 *
 *  $Id$
 *
 *  Get full path name of a file functions.
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
