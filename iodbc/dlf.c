/*
 *  dlf.c
 *
 *  $Id$
 *
 *  Dynamic Library Loader (mapping to SVR4)
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com> 
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

#include <dlf.h>
#include <errno.h>

#ifdef	DLDAPI_DEFINED
#undef DLDAPI_DEFINED
#endif

#ifdef	DLDAPI_SVR4_DLFCN
#define DLDAPI_DEFINED
static char sccsid[] = "@(#)dynamic load interface -- SVR4 dlfcn";
#endif

/********************************* 
 *
 *	HP/UX 
 *
 *********************************/

#ifdef	DLDAPI_HP_SHL
#define	DLDAPI_DEFINED

#include <dl.h>

static char sccsid[] = "@(#)dynamic load interface -- HP/UX dl(shl)";

void *
dlopen (char *path, int mode)
{
  return (void *) shl_load ((char *) (path), BIND_IMMEDIATE | BIND_NONFATAL, 0L);
}


void *
dlsym (void *hdll, char *sym)
{
  void *symaddr = 0;
  int ret;

#if 0
  /*
   *  iODBC does not need a handle to itself
   */
  if (!hdll)
    hdll = (void *) PROG_HANDLE;
#endif

  /* Remember, a driver may export calls as function pointers 
   * (i.e. with type TYPE_DATA) rather than as functions 
   * (i.e. with type TYPE_PROCEDURE). Thus, to be safe, we 
   * uses TYPE_UNDEFINED to cover all of them. 
   */
  ret = shl_findsym ((shl_t *) & hdll, sym, TYPE_UNDEFINED, &symaddr);

  if (ret == -1)
    return 0;

  return symaddr;
}


char *
dlerror ()
{
  extern char *strerror ();

  return strerror (errno);
}


int
dlclose (void *hdll)
{
  struct shl_descriptor d;

  /*
   *  As HP/UX does not use a reference counter for unloading, 
   *  we can only unload the driver when it is loaded once.
   */
  if (shl_gethandle_r ((shl_t) hdll, &d) < 0 || d.ref_count > 1)
  {
    return 0;
  }

  return shl_unload ((shl_t) hdll);
}
#endif /* end of HP/UX Section */


/*********************************
 *
 *	IBM AIX
 *
 *********************************/

#ifdef	DLDAPI_AIX_LOAD
#define	DLDAPI_DEFINED

#include <sys/types.h>
#include <sys/ldr.h>
#include <sys/stat.h>
#include <nlist.h>

#ifndef	HTAB_SIZE
#define	HTAB_SIZE	256
#endif

#define	FACTOR		0.618039887	/* i.e. (sqrt(5) - 1)/2 */

#ifndef	ENTRY_SYM
#define	ENTRY_SYM	".__start"	/* default entry point for aix */
#endif

typedef struct slot_s
{
  char *sym;
  long fdesc[3];		/* 12 bytes function descriptor */
  struct slot_s *next;
}
slot_t;

/* Note: on AIX, a function pointer actually points to a
 * function descriptor, a 12 bytes data. The first 4 bytes
 * is the virtual address of the function. The next 4 bytes 
 * is the virtual address of TOC (Table of Contents) of the
 * object module the function belong to. The last 4 bytes 
 * are always 0 for C and Fortran functions. Every object 
 * module has an entry point (which can be specified at link
 * time by -e ld option). iODBC driver manager requires ODBC
 * driver shared library always use the default entry point
 * (so you shouldn't use -e ld option when creating a driver
 * share library). load() returns the function descriptor of 
 * a module's entry point. From which we can calculate function
 * descriptors of other functions in the same module by using
 * the fact that the load() doesn't change the relative 
 * offset of functions to their module entry point(i.e the 
 * offset in memory loaded by load() will be as same as in 
 * the module library file). 
 */

typedef slot_t *hent_t;
typedef struct nlist nlist_t;
typedef struct stat stat_t;

typedef struct obj
  {
    int dev;			/* device id */
    int ino;			/* inode number */
    char *path;			/* file name */
    int (*pentry) ();		/* entry point of this share library */
    int refn;			/* number of reference */
    hent_t htab[HTAB_SIZE];
    struct obj *next;
  }
obj_t;

static char *errmsg = 0;

static void
init_htab (hent_t * ht)
/* initate a hashing table */
{
  int i;

  for (i = 0; i < HTAB_SIZE; i++)
    ht[i] = (slot_t *) 0;

  return;
}


static void
clean_htab (hent_t * ht)
/* free all slots */
{
  int i;
  slot_t *ent;
  slot_t *tent;

  for (i = 0; i < HTAB_SIZE; i++)
    {
      for (ent = ht[i]; ent;)
	{
	  tent = ent->next;

	  free (ent->sym);
	  free (ent);

	  ent = tent;
	}

      ht[i] = 0;
    }

  return;
}


static int
hash (char *sym)
{
  int a, key;
  double f;

  if (!sym || !*sym)
    return 0;

  for (key = *sym; *sym; sym++)
    {
      key += *sym;
      a = key;

      key = (int) ((a << 8) + (key >> 8));
      key = (key > 0) ? key : -key;
    }

  f = key * FACTOR;
  a = (int) f;

  return (int) ((HTAB_SIZE - 1) * (f - a));
}


static hent_t
search (hent_t * htab, char *sym)
/* search hashing table to find a matched slot */
{
  int key;
  slot_t *ent;

  key = hash (sym);

  for (ent = htab[key]; ent; ent = ent->next)
    {
      if (!strcmp (ent->sym, sym))
	return ent;
    }

  return 0;			/* no match */
}


static void
insert (hent_t * htab, slot_t * ent)
/* insert a new slot to hashing table */
{
  int key;

  key = hash (ent->sym);

  ent->next = htab[key];
  htab[key] = ent;

  return;
}


static slot_t *
slot_alloc (char *sym)
/* allocate a new slot with symbol */
{
  slot_t *ent;

  ent = (slot_t *) malloc (sizeof (slot_t));

  ent->sym = (char *) malloc (strlen (sym) + 1);

  if (!ent->sym)
    {
      free (ent);
      return 0;
    }

  strcpy (ent->sym, sym);

  return ent;
}


static obj_t *obj_list = 0;

void *
dlopen (char *file, int mode)
{
  stat_t st;
  obj_t *pobj;
  char buf[1024];

  if (!file || !*file)
    {
      errno = EINVAL;
      return 0;
    }

  errno = 0;
  errmsg = 0;

  if (stat (file, &st))
    return 0;

  for (pobj = obj_list; pobj; pobj = pobj->next)
    /* find a match object */
    {
      if (pobj->ino == st.st_ino
	  && pobj->dev == st.st_dev)
	{
	  /* found a match. increase its
	   * reference count and return 
	   * its address */
	  pobj->refn++;
	  return pobj;
	}
    }

  pobj = (obj_t *) malloc (sizeof (obj_t));

  if (!pobj)
    return 0;

  pobj->path = (char *) malloc (strlen (file) + 1);

  if (!pobj->path)
    {
      free (pobj);
      return 0;
    }

  strcpy (pobj->path, file);

  pobj->dev = st.st_dev;
  pobj->ino = st.st_ino;
  pobj->refn = 1;

  pobj->pentry = (int (*)()) load (file, 0, 0);

  if (!pobj->pentry)
    {
      free (pobj->path);
      free (pobj);
      return 0;
    }

  init_htab (pobj->htab);

  pobj->next = obj_list;
  obj_list = pobj;

  return pobj;
}


int
dlclose (void *hobj)
{
  obj_t *pobj = (obj_t *) hobj;
  obj_t *tpobj;
  int match = 0;

  if (!hobj)
    {
      errno = EINVAL;
      return -1;
    }

  errno = 0;
  errmsg = 0;

  if (pobj == obj_list)
    {
      pobj->refn--;

      if (pobj->refn)
	return 0;

      match = 1;
      obj_list = pobj->next;
    }

  for (tpobj = obj_list; !match && tpobj; tpobj = tpobj->next)
    {
      if (tpobj->next == pobj)
	{
	  pobj->refn--;

	  if (pobj->refn)
	    return 0;

	  match = 1;
	  tpobj->next = pobj->next;
	}
    }

  if (match)
    {
      unload ((void *) (pobj->pentry));
      clean_htab (pobj->htab);
      free (pobj->path);
      free (pobj);
    }

  return 0;
}


char *
dlerror ()
{
  extern char *sys_errlist[];

  if (!errmsg || !errmsg[0])
    {
      if (errno >= 0)
	return sys_errlist[errno];

      return "";
    }

  return errmsg;
}


void *
dlsym (void *hdl, char *sym)
{
  nlist_t nl[3];
  obj_t *pobj = (obj_t *) hdl;
  slot_t *ent;
  int (*fp) ();
  long lbuf[3];

  if (!hdl || !(pobj->htab) || !sym || !*sym)
    {
      errno = EINVAL;
      return 0;
    }

  errno = 0;
  errmsg = 0;

  ent = search (pobj->htab, sym);

  if (ent)
    return ent->fdesc;

#define	n_name	_n._n_name

  nl[0].n_name = ENTRY_SYM;
  nl[1].n_name = sym;
  nl[2].n_name = 0;

  /* There is a potential problem here. If application
   * did not pass a full path name, and changed the
   * working directory after the load(), then nlist()
   * will be unable to open the original shared library
   * file to resolve the symbols. there are 3 ways to working 
   * round this: 1. convert to full pathname in driver
   * manager. 2. applications always pass driver's full 
   * path name. 3. if driver itself don't support 
   * SQLGetFunctions(), call it with SQL_ALL_FUNCTIONS
   * as flag immidately after SQLConnect(), SQLDriverConnect()
   * and SQLBrowseConnect() to force the driver manager
   * resolving all will be used symbols. 
   */
  if (nlist (pobj->path, nl) == -1)
    return 0;

  if (!nl[0].n_type && !nl[0].n_value)
    {
      errmsg = "can't locate module entry symbol";
      return 0;
    }

  /* Note: On AIX 3.x if the object library is not
   * built with -g compiling option, .n_type field
   * is always 0. While on 4.x it will be 32. 
   * On AIX 4.x, if the symbol is a entry point,
   * n_value will be 0. However, one thing is for sure 
   * that if a symbol is not existance in the file,
   * both .n_type and .n_value would be 0.
   */

  if (!nl[1].n_type && !nl[1].n_value)
    {
      errmsg = "symbol not existance in this module";
      return 0;
    }

  ent = slot_alloc (sym);

  if (!ent)
    return 0;

  /* catch it with a slot in the hashing table */
  insert (pobj->htab, ent);

  memcpy (ent->fdesc, pobj->pentry, sizeof (ent->fdesc));

  /* now ent->fdesc[0] is the virtual address of entry point
   * and ent->fdesc[1] is the TOC of the module
   */

  /* let's calculate the virtual address of the symbol 
   * by adding a relative offset getting from the module 
   * file symbol table, i.e
   *
   *  functin virtual address = entry point virtual address +
   *     + ( function offset in file - entry point offset in file )
   */

  (ent->fdesc)[0] = (ent->fdesc)[0] +
      (nl[1].n_value - nl[0].n_value);

  /* return the function descriptor */
  return ent->fdesc;
}
#endif /* end of IBM AIX Section */


/********************************* 
 *
 *	Windows 3.x, 95, NT
 *
 *********************************/

#ifdef	DLDAPI_WINDOWS
#define	DLDAPI_DEFINED

#include <windows.h>

void FAR *
dlopen (char FAR * dll, int mode)
{
  HINSTANCE hint;

  if (dll == NULL)
    {
      return GetWindowWord (NULL, GWW_HINSTANCE);
    }

  hint = LoadLibrary (dll);

  if (hint < HINSTANCE_ERROR)
    {
      return NULL;
    }

  return (void FAR *) hint;
}


void FAR *
dlsym (void FAR * hdll, char FAR * sym)
{
  return (void FAR *) GetProcAddress (hdll, sym);
}


char FAR *
dlerror ()
{
  return 0L;			/* unimplemented yet */
}


int
dlclose (void FAR * hdll)
{
  FreeLibrary ((HINSTANCE) hdll);
}
#endif /* end of Windows family */


/***********************************
 *
 * 	VMS
 *
 ***********************************/

#ifdef VMS
#define	DLDAPI_DEFINED
#ifdef DLDAPI_VMS_IODBC

#include <stdio.h>
#include <descrip.h>
#include <starlet.h>
#include <ssdef.h>
#include <libdef.h>
#include <lib$routines>
#include <rmsdef.h>
#include <fabdef.h>
#include <namdef.h>

#ifndef LIB$M_FIS_MIXCASE
#define LIB$M_FIS_MIXCASE 1<<4
#endif

typedef struct
{
  struct dsc$descriptor_s filename_d;
  struct dsc$descriptor_s image_d;
  char filename[NAM$C_MAXRSS];	/* $PARSEd image name */
}
dll_t;

/*
 *  The following static int contains the last VMS error returned. It is kept
 *  static so that dlerror() can get it. This method is dangerous if you have
 *  threaded applications, but this is the way the UNIX dlopen() etc
 *  is defined.
 */
static int saved_status = SS$_NORMAL;
static char dlerror_buf[256];


static int
iodbc_find_image_symbol (
    struct dsc$descriptor_s *filename_d,
    struct dsc$descriptor_s *symbol_d,
    void **rp,
    struct dsc$descriptor_s *image_d, int flag)
{
  lib$establish (lib$sig_to_ret);
  return lib$find_image_symbol (filename_d, symbol_d, rp, image_d, flag);
}


void *
iodbc_dlopen (char *path, int unused_flag)
{
  int status;
  dll_t *dll;
  struct FAB imgfab;
  struct NAM imgnam;
  static char defimg[] = "SYS$SHARE:.EXE";

  if (path == NULL)
    {
      saved_status = SS$_UNSUPPORTED;
      return NULL;
    }

  dll = malloc (sizeof (dll_t));
  if (dll == NULL)
    {
      saved_status = SS$_INSFMEM;
      return NULL;
    }

  imgfab = cc$rms_fab;
  imgfab.fab$l_fna = path;
  imgfab.fab$b_fns = strlen (path);
  imgfab.fab$w_ifi = 0;
  imgfab.fab$l_dna = defimg;
  imgfab.fab$b_dns = sizeof (defimg);
  imgfab.fab$l_fop = FAB$M_NAM;
  imgfab.fab$l_nam = &imgnam;
  imgnam = cc$rms_nam;
  imgnam.nam$l_esa = dll->filename;
  imgnam.nam$b_ess = NAM$C_MAXRSS;
  status = sys$parse (&imgfab);
  if (!(status & 1))
    {
      free (dll);
      saved_status = status;
      return NULL;
    }

  dll->filename_d.dsc$b_dtype = DSC$K_DTYPE_T;
  dll->filename_d.dsc$b_class = DSC$K_CLASS_S;
  dll->filename_d.dsc$a_pointer = imgnam.nam$l_name;
  dll->filename_d.dsc$w_length = imgnam.nam$b_name;
  dll->image_d.dsc$b_dtype = DSC$K_DTYPE_T;
  dll->image_d.dsc$b_class = DSC$K_CLASS_S;
  dll->image_d.dsc$a_pointer = dll->filename;
  dll->image_d.dsc$w_length = imgnam.nam$b_esl;

  /*
   *  VMS does not have the concept of first opening a shared library and then
   *  asking for symbols; the LIB$FIND_IMAGE_SYMBOL routine does both.
   *  Since I want my implementation of dlopen() to return an error if the
   *  shared library can not be loaded, I try to find a dummy symbol in the
   *  library.
   */
  iodbc_dlsym (dll, "THIS_ROUTINE_MIGHT_NOT_EXIST");
  if (!((saved_status ^ LIB$_KEYNOTFOU) & ~7))
    {
      saved_status = SS$_NORMAL;
    }
  if (saved_status & 1)
    {
      return dll;
    }
  else
    {
      free (dll);
      return NULL;
    }
}


void *
iodbc_dlsym (void *hdll, char *sym)
{
  int status;
  dll_t *dll;
  struct dsc$descriptor_s symbol_d;
  void *rp;

  dll = hdll;
  if (dll == NULL)
    return NULL;

  symbol_d.dsc$b_dtype = DSC$K_DTYPE_T;
  symbol_d.dsc$b_class = DSC$K_CLASS_S;
  symbol_d.dsc$a_pointer = sym;
  symbol_d.dsc$w_length = strlen (sym);
  status = iodbc_find_image_symbol (&dll->filename_d, &symbol_d, &rp,
      &dll->image_d, 0);
  if (!((saved_status ^ LIB$_KEYNOTFOU) & ~7))
    {
      status = iodbc_find_image_symbol (&dll->filename_d, &symbol_d, &rp,
	  &dll->image_d, LIB$M_FIS_MIXCASE);
    }
  if (status & 1)
    {
      return rp;
    }
  else
    {
      saved_status = status;
      return NULL;
    }
}


char *
iodbc_dlerror ()
{
  struct dsc$descriptor desc;
  short outlen;
  int status;

  if (saved_status & 1)
    {
      return NULL;
    }

  desc.dsc$b_dtype = DSC$K_DTYPE_T;
  desc.dsc$b_class = DSC$K_CLASS_S;
  desc.dsc$a_pointer = dlerror_buf;
  desc.dsc$w_length = sizeof (dlerror_buf);
  status = sys$getmsg (saved_status, &outlen, &desc, 15, 0);
  if (status & 1)
    {
      dlerror_buf[outlen] = '\0';
    }
  else
    {
      sprintf (dlerror_buf, "Message number %8X", saved_status);
    }
  saved_status = SS$_NORMAL;
  return (dlerror_buf);
}


int
iodbc_dlclose (void *hdll)
{
  /*
   *  Not really implemented since VMS doesn't support unloading images.
   *  The hdll pointer is released though.
   */
  free (hdll);
  return 0;
}
#endif /* DLDAPI_VMS_IODBC */
#endif /* VMS */


/********************************* 
 *
 *	Apple MacOS X Rhapsody
 *
 *********************************/
#ifdef	DLDAPI_DYLD
#define	DLDAPI_DEFINED
#include <stdio.h>
#include <mach-o/dyld.h>

static char sccsid[] = "@(#)dynamic load interface -- MacOS X dl(dyld)";


static void
undefined_symbol_handler (const char *symbolName)
{
  fprintf (stderr, "dyld found undefined symbol: %s\n", symbolName);

  abort ();
}


static NSModule 
multiple_symbol_handler (NSSymbol s, NSModule old, NSModule new)
{
  /*
   *  Since we can't unload symbols, we're going to run into this
   *  every time we reload a module. Workaround here is to just
   *  rebind to the new symbol, and forget about the old one.
   *  This is crummy, because it's basically a memory leak.
   */

  return (new);
}


static void
linkEdit_symbol_handler (NSLinkEditErrors c, int errorNumber,
    const char *fileName, const char *errorString)
{
  fprintf (stderr, "dyld errors during link edit for file %s\n%s\n",
      fileName, errorString);

  abort ();
}


void *
dlopen (char *path, int mode)
{
  NSObjectFileImage image;
  NSLinkEditErrorHandlers handlers;
  NSModule handle = NULL;
  int i;

  /*
   *  Install error handler 
   */
  handlers.undefined = undefined_symbol_handler;
#if !defined (NSLINKMODULE_OPTION_PRIVATE)
  handlers.multiple = multiple_symbol_handler;
#endif
  handlers.linkEdit = linkEdit_symbol_handler;

  NSInstallLinkEditErrorHandlers (&handlers);

  /*
   *  Load object
   */
  i = NSCreateObjectFileImageFromFile (path, &image);
  if (i != NSObjectFileImageSuccess)
    {
      static char *ErrorStrings[] =
      {
	  "%s(%d): Object Image Load Failure\n",
	  "%s(%d): Object Image Load Success\n",
	  "%s(%d): Not an recognisable object file\n",
	  "%s(%d): No valid architecture\n",
	  "%s(%d): Object image has an invalid format\n",
	  "%s(%d): Invalid access (permissions?)\n",
	  "%s(%d): Unknown error code from NSCreateObjectFileImageFromFile\n",
      };

      if (i < 0 || i > 6)
	i = 6;

      fprintf (stderr, ErrorStrings[i], path, i);
    }
  else
    {
#if !defined (NSLINKMODULE_OPTION_PRIVATE)
      handle = NSLinkModule (image, path, TRUE);
#else
      handle = NSLinkModule (image, path, NSLINKMODULE_OPTION_PRIVATE);
#endif
    }

  return (void *) handle;
}


void *
dlsym (void *hdll, char *sym)
{
  NSSymbol symbol;

#if !defined (NSLINKMODULE_OPTION_PRIVATE)
  if (NSIsSymbolNameDefined (sym))
    {
      symbol = NSLookupAndBindSymbol (sym);

      return NSAddressOfSymbol (symbol);
    }

  return NULL;
#else
  symbol = NSLookupSymbolInModule ((NSModule) hdll, sym);

  return NSAddressOfSymbol (symbol);   
#endif
}


char *
dlerror ()
{
  return NULL;
}


int 
dlclose (void *hdll)
{
  NSUnLinkModule (hdll, FALSE);
  return 0;
}
#endif	/* end of Rhapsody Section */


/***********************************
 *
 * 	other platforms
 *
 ***********************************/

#ifdef DLDAPI_OS2
#define	DLDAPI_DEFINED
/*
 *    DosLoadModule(), DosQueryProcAddress(), DosFreeModule(), ...
 */
#endif

#ifdef	DLDAPI_MAC
#define	DLDAPI_DEFINED
#endif

#ifdef DLDAPI_NEXT
#define	DLDAPI_DEFINED
#endif

#ifndef DLDAPI_DEFINED
#error	"dynamic load editor undefined"
#endif
