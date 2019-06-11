/*
 *  execute.c
 *
 *  $Id$
 *
 *  Invoke a query
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
 *  Copyright (C) 1996-2019 by OpenLink Software <iodbc@openlinksw.com>
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

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>

#include <unicode.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>

void
_iodbcdm_do_cursoropen (STMT_t * pstmt)
{
  SQLRETURN retcode;
  SWORD ncol;

  pstmt->state = en_stmt_executed;

  retcode = _iodbcdm_NumResultCols ((SQLHSTMT) pstmt, &ncol);

  if (SQL_SUCCEEDED (retcode))
    {
      if (ncol)
	{
	  pstmt->state = en_stmt_cursoropen;
	  pstmt->cursor_state = en_stmt_cursor_opened;
	}
      else
	{
	  pstmt->state = en_stmt_executed;
	  pstmt->cursor_state = en_stmt_cursor_no;
	}
    }
}


#if (ODBCVER >= 0x0300)

SQLLEN
_iodbcdm_OdbcCTypeSize (SWORD fCType)
{
  SQLLEN cbSize = 0;

  switch (fCType)
  {
    /*
     * ODBC fixed length types
     */
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
      cbSize = sizeof (SQLSMALLINT);
      break;

    case SQL_C_USHORT:
      cbSize = sizeof (SQLUSMALLINT);
      break;

    case SQL_C_LONG:
    case SQL_C_SLONG:
      cbSize = sizeof (SQLINTEGER);
      break;

    case SQL_C_ULONG:
      cbSize = sizeof (SQLUINTEGER);
      break;

    case SQL_C_FLOAT:
      cbSize = sizeof (SQLREAL);
      break;

    case SQL_C_DOUBLE:
      cbSize = sizeof (SQLDOUBLE);
      break;

    case SQL_C_BIT:
      cbSize = sizeof (SQLCHAR);
      break;

    case SQL_TINYINT:
      cbSize = sizeof (SQLSCHAR);
      break;

    case SQL_C_STINYINT:
      cbSize = sizeof (SQLSCHAR);
      break;

    case SQL_C_UTINYINT:
      cbSize = sizeof (SQLCHAR);
      break;

    case SQL_C_DATE:
#if (ODBCVER >= 0x0300)
      cbSize = sizeof (SQL_DATE_STRUCT);
#else
      cbSize = sizeof (DATE_STRUCT);
#endif
      break;

    case SQL_C_TIME:
#if (ODBCVER >= 0x0300)
      cbSize = sizeof (SQL_TIME_STRUCT);
#else
      cbSize = sizeof (TIME_STRUCT);
#endif
      break;

    case SQL_C_TIMESTAMP:
#if (ODBCVER >= 0x0300)
      cbSize = sizeof (SQL_TIMESTAMP_STRUCT);
#else
      cbSize = sizeof (TIMESTAMP_STRUCT);
#endif
      break;

#if (ODBCVER >= 0x0300)
#ifdef ODBCINT64
    case SQL_C_SBIGINT:
      cbSize = sizeof (SQLBIGINT);
      break;
    case SQL_C_UBIGINT:
      cbSize = sizeof (SQLUBIGINT);
      break;
#endif /* ODBCINT64 */

    case SQL_C_TYPE_DATE:
      cbSize = sizeof (SQL_DATE_STRUCT);
      break;

    case SQL_C_TYPE_TIME:
      cbSize = sizeof (SQL_TIME_STRUCT);
      break;

    case SQL_C_TYPE_TIMESTAMP:
      cbSize = sizeof (SQL_TIMESTAMP_STRUCT);
      break;

    case SQL_C_NUMERIC:
      cbSize = sizeof (SQL_NUMERIC_STRUCT);
      break;

    case SQL_C_GUID:
      cbSize = sizeof (SQLGUID);
      break;
#endif /* ODBCVER >= 0x0300 */
    /*
     * Variable length types and unsupported types
     */
    case SQL_C_CHAR:
    case SQL_C_BINARY:
    case SQL_C_WCHAR:
    default:
      break;
  }

  return cbSize;
}


static SQLLEN
GetElementSize (PPARM pparm, DM_CONV *conv)
{
  SQLLEN elementSize;


  if (pparm->pm_c_type == SQL_C_CHAR || pparm->pm_c_type == SQL_C_BINARY)
    {
      elementSize = pparm->pm_cbValueMax == 0
	  ? pparm->pm_precision : pparm->pm_cbValueMax;

      elementSize = (elementSize == 0) ? sizeof(SQLLEN) : elementSize;
    }	
  else if (pparm->pm_c_type == SQL_C_WCHAR)
    {
      if (pparm->pm_cbValueMax == 0)
        {
          if (conv && conv->dm_cp != conv->drv_cp)
            elementSize = pparm->pm_precision * DM_WCHARSIZE(conv);
          else
            elementSize = pparm->pm_precision * sizeof(wchar_t);
        }
      else
        elementSize = pparm->pm_cbValueMax;

      elementSize = (elementSize == 0) ? sizeof(SQLLEN) : elementSize;
    }
  else				/* fixed length types */
    {
      /*elementSize = pparm->pm_size;*/
      elementSize = _iodbcdm_OdbcCTypeSize(pparm->pm_c_type);
    }

  return elementSize;
}


/* DM=>DRV */
static void 
_ExecConv_W2A(wchar_t *wdata, SQLLEN *pInd, UDWORD size, DM_CONV *conv)
{
  char *buf;

  if (*pInd != SQL_NULL_DATA && *pInd != SQL_DATA_AT_EXEC && *pInd > SQL_LEN_DATA_AT_EXEC_OFFSET)
    {
      buf = (char*) conv_text_m2d(conv, wdata, (ssize_t)*pInd, CD_W2A);
      if (buf != NULL)
	strcpy((char*)wdata, buf);

      MEM_FREE (buf);

      if (pInd && *pInd != SQL_NTS)
        {
          if (conv->drv_cp == CP_UTF8)
	    *pInd = strlen((char*)wdata);
          else
	    *pInd /= DRV_WCHARSIZE(conv);
	}
    }
}


/* DRV => DM */
static void 
_ExecConv_A2W(char *data, SQLLEN *pInd, UDWORD size, DM_CONV *conv)
{
  wchar_t *buf;

  if (*pInd != SQL_NULL_DATA && *pInd != SQL_DATA_AT_EXEC && *pInd > SQL_LEN_DATA_AT_EXEC_OFFSET)
    {
      buf = (wchar_t*) conv_text_d2m(conv, data, (ssize_t)*pInd, CD_A2W);
      if (buf != NULL)
	DM_WCSCPY (conv, data, buf);

      MEM_FREE (buf);

      if (pInd && *pInd != SQL_NTS)
        {
          if (conv->dm_cp == CP_UTF8)
	    *pInd = strlen(data);
          else
	    *pInd *= DM_WCHARSIZE(conv);
	}
    }
}


static void 
_ExecConv_W2W(char *data, SQLLEN *pInd, UDWORD size, DM_CONV *conv, 
	BOOL bOutput)
{
  void *buf = NULL;

  if (*pInd != SQL_NULL_DATA && *pInd != SQL_DATA_AT_EXEC && *pInd > SQL_LEN_DATA_AT_EXEC_OFFSET)
    {
      if (bOutput)
        {
          /* DRV => DM */
          void *buf = conv_text_d2m(conv, data, (ssize_t)*pInd, CD_W2W);
          if (buf != NULL)
            {
	      DM_WCSNCPY (conv, data, buf, size/DM_WCHARSIZE(conv));
	      if (conv->dm_cp == CP_UTF8)
	        data[size-1] = 0;
	      else
                DM_SetWCharAt(conv, data, size/DM_WCHARSIZE(conv)-1, 0);
	    }

          if (pInd && *pInd != SQL_NTS)
            {
	      if (conv->dm_cp == CP_UTF8)
	        *pInd = strlen(data);
	      else
	        *pInd = DM_WCSLEN(conv, data) * DM_WCHARSIZE(conv);
	    }
        }
      else
        {
          /* DM => DRV */
          void *buf = conv_text_m2d(conv, data, (ssize_t)*pInd, CD_W2W);
          if (buf != NULL)
            {
              DRV_WCSNCPY(conv, data, buf, size/DRV_WCHARSIZE(conv));
              if (conv->drv_cp == CP_UTF8)
                data[size-1] = 0;
              else
                DRV_SetWCharAt(conv, data, size/DRV_WCHARSIZE(conv)-1, 0);
            }

          if (pInd && *pInd != SQL_NTS)
            {
	      if (conv->drv_cp == CP_UTF8)
	        *pInd = strlen(data);
	      else
	        *pInd = DRV_WCSLEN(conv, data) * DRV_WCHARSIZE(conv);
	    }
        }

       MEM_FREE (buf);
    }
}


static SQLRETURN
_ConvParam (STMT_t *pstmt, PPARM pparm, SQLULEN row, BOOL bOutput, 
	DM_CONV *conv, SWORD unicode_driver)
{
  SQLLEN octetLen;
  void *value;
  SQLLEN *pInd = NULL;
  SQLLEN elementSize = 0;
  SQLUINTEGER bindOffset = pstmt->param_bind_offset;

  if (pparm->pm_c_type != SQL_C_WCHAR)
    return SQL_SUCCESS;

  elementSize = GetElementSize (pparm, conv);

  if (pstmt->param_bind_type)
    {
      /* row-wise binding of parameters in force */
      if (pparm->pm_pOctetLength)
	octetLen = *(SQLLEN *) ((char *) pparm->pm_pOctetLength
	    + row * pstmt->param_bind_type + bindOffset);
      else
        octetLen = pparm->pm_size;

      if (pparm->pm_pInd)
        pInd = (SQLLEN *) ((char *) pparm->pm_pInd
	        + row * pstmt->param_bind_type + bindOffset);
    }
  else
    {
      octetLen = pparm->pm_pOctetLength ? 
                 ((SQLLEN*)((char*)pparm->pm_pOctetLength + bindOffset))[row] : 
                 pparm->pm_size;
      if (pparm->pm_pInd)
        pInd = &((SQLLEN*)((char*)pparm->pm_pInd + bindOffset))[row];
    }

  if (!pInd || (pInd && *pInd == SQL_NULL_DATA ))
    {
      return SQL_SUCCESS;
    }

  if (octetLen == SQL_DATA_AT_EXEC || octetLen <= SQL_LEN_DATA_AT_EXEC_OFFSET)
    {
      value = NULL;
      PUSHSQLERR (pstmt->herr, en_HYC00);  /* Unsupported Modes */
      return SQL_ERROR;
    }
  else
    value = pparm->pm_data;

  if (value == NULL)
    return 0;

  if (pstmt->param_bind_type)
    /* row-wise binding of parameters in force */
    value = (char *) pparm->pm_data + row * pstmt->param_bind_type + bindOffset;
  else
    value = (char *) pparm->pm_data + row * elementSize + bindOffset;

  if (unicode_driver)
    _ExecConv_W2W(value, pInd, elementSize, conv, bOutput);
  else
    {
      if (bOutput)
        _ExecConv_A2W(value, pInd, elementSize, conv);
      else
        _ExecConv_W2A(value, pInd, elementSize, conv);
    }
  return SQL_SUCCESS;

}


static SQLRETURN
_ConvRebindedParam (STMT_t *pstmt, PPARM pparm, SQLULEN row, BOOL bOutput, 
	DM_CONV *conv)
{
  SQLLEN octetLen;
  void *val_dm;
  void *val_drv;
  SQLLEN *pInd_dm = NULL;
  SQLLEN *pInd_drv = NULL;
  SQLLEN elementSize = 0;
  IODBC_CHARSET m_charset = (conv) ? conv->dm_cp : CP_DEF;
  IODBC_CHARSET d_charset = (conv) ? conv->drv_cp : CP_DEF;
  SQLUINTEGER bindOffset = pstmt->param_bind_offset;

  elementSize = GetElementSize (pparm, conv);

  if (pstmt->param_bind_type)
    {
      /* row-wise binding of parameters in force */
      if (pparm->pm_pOctetLength)
	octetLen = *(SQLLEN *) ((char *) pparm->pm_pOctetLength
	    + row * pstmt->param_bind_type + bindOffset);
      else
        octetLen = pparm->pm_size;

      if (pparm->pm_pInd)
        {
          pInd_dm = (SQLLEN *) ((char *) pparm->pm_pInd
	        + row * pstmt->param_bind_type + bindOffset);
          pInd_drv = (SQLLEN *) ((char *) pparm->pm_conv_pInd
	        + row * pstmt->conv_param_bind_type);
	}
    }
  else
    {
      octetLen = pparm->pm_pOctetLength ? 
                 ((SQLLEN*)((char*)pparm->pm_pOctetLength + bindOffset))[row] : 
                 pparm->pm_size;
      if (pparm->pm_pInd)
        {
          pInd_dm = &((SQLLEN*)((char*)pparm->pm_pInd + bindOffset))[row];
          pInd_drv = &((SQLLEN*)pparm->pm_conv_pInd)[row];
        }
    }

  if (!pInd_dm)
    return SQL_SUCCESS;

  if (pstmt->param_bind_type && (octetLen == SQL_DATA_AT_EXEC || octetLen <= SQL_LEN_DATA_AT_EXEC_OFFSET))
    {
      PUSHSQLERR (pstmt->herr, en_HYC00);  /* Unsupported Modes */
      return SQL_ERROR;
    }

  if (bOutput && (octetLen == SQL_DATA_AT_EXEC || octetLen <= SQL_LEN_DATA_AT_EXEC_OFFSET))
    {
      PUSHSQLERR (pstmt->herr, en_HYC00);  /* Unsupported Modes */
      return SQL_ERROR;
    }

  
  if (bOutput && *pInd_drv == SQL_NULL_DATA
     && (pparm->pm_usage == SQL_PARAM_OUTPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
    {
      *pInd_dm = *pInd_drv;
      return SQL_SUCCESS;
    }
  else if (!bOutput && (*pInd_dm == SQL_NULL_DATA || *pInd_dm == SQL_DATA_AT_EXEC || *pInd_dm <= SQL_LEN_DATA_AT_EXEC_OFFSET)
     && (pparm->pm_usage == SQL_PARAM_INPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT)) 
    { 
      *pInd_drv = *pInd_dm;
      return SQL_SUCCESS;
    }
  
  
  if (pparm->pm_data == NULL) 
    return SQL_SUCCESS;

  if (pstmt->param_bind_type)
    {
      /* row-wise binding of parameters in force */
      val_dm = (char *) pparm->pm_data + row * pstmt->param_bind_type + bindOffset;
      val_drv = (char *) pparm->pm_conv_data + row * pstmt->conv_param_bind_type;
    }
  else
    {
      val_dm = (char *) pparm->pm_data + row * elementSize + bindOffset;
      val_drv = (char *) pparm->pm_conv_data + row * pparm->pm_conv_el_size;
    }

  if (pparm->pm_c_type_orig == SQL_C_WCHAR)
    {
      SQLLEN len, size;
      if (bOutput && (pparm->pm_usage == SQL_PARAM_OUTPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
        {
          /* drv-dm */
          len = (SQLLEN)*pInd_drv;

          size = dm_conv_W2W(val_drv, len, val_dm, elementSize, 
              d_charset, m_charset);
          if (m_charset == CP_UTF8)
            *(char*)(val_dm + size) = 0;
          else
            DM_SetWCharAt(conv, val_dm, size/DM_WCHARSIZE(conv), 0);

	  *pInd_dm = (*pInd_drv != SQL_NTS)? size: SQL_NTS;
        }
      else if (!bOutput && (pparm->pm_usage == SQL_PARAM_INPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
        {
          /* dm-drv */
          len = (SQLLEN)*pInd_dm;

          size = dm_conv_W2W(val_dm, len, val_drv, pparm->pm_conv_el_size, 
              m_charset, d_charset);
          if (d_charset == CP_UTF8)
            *(char*)(val_drv + size) = 0;
          else
            DRV_SetWCharAt(conv, val_drv, size/DRV_WCHARSIZE(conv), 0);

          *pInd_drv = (*pInd_dm != SQL_NTS)? size: SQL_NTS;
        }
    }
  else
    {
      if (bOutput && (pparm->pm_usage == SQL_PARAM_OUTPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
        {
          memcpy(val_dm, val_drv, elementSize);

          if (pInd_dm)
            *pInd_dm = *pInd_drv;
        }
      else if (!bOutput && (pparm->pm_usage == SQL_PARAM_INPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
        {
          memcpy(val_drv, val_dm, elementSize);

          if (pInd_dm)
            *pInd_drv = *pInd_dm;
        }
    }

  return SQL_SUCCESS;

}


static SQLRETURN
_ReBindParam (STMT_t *pstmt, PPARM pparm)
{
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;
  SQLRETURN retcode = SQL_SUCCESS;
  SQLSMALLINT nSqlType;


  hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_BindParameter);
#if (ODBCVER >=0x0300)
  hproc3 = _iodbcdm_getproc (pstmt->hdbc, en_BindParam);
#endif

  nSqlType = _iodbcdm_map_sql_type (pparm->pm_sql_type, penv->dodbc_ver);


  if (odbc_ver == SQL_OV_ODBC2 && 
      (  dodbc_ver == SQL_OV_ODBC2
       || (dodbc_ver == SQL_OV_ODBC3 && hproc2 != SQL_NULL_HPROC)))
    hproc3 = SQL_NULL_HPROC;

#if (ODBCVER >=0x0300)
  if (pparm->pm_usage == SQL_PARAM_INPUT && hproc2 == SQL_NULL_HPROC 
      && hproc3 != SQL_NULL_HPROC)
    {
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
	      (pstmt->dhstmt, 
	       pparm->pm_par, 
	       pparm->pm_c_type, 
	       nSqlType, 
	       pparm->pm_precision, 
	       pparm->pm_scale, 
	       pparm->pm_conv_data, 
	       pparm->pm_conv_pInd));
    }
  else
#endif
    {
      if (hproc2 == SQL_NULL_HPROC)
        {
          PUSHSQLERR (pstmt->herr, en_IM001);
          return SQL_ERROR;
        }
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
          (pstmt->dhstmt, 
           pparm->pm_par, 
           pparm->pm_usage, 
           pparm->pm_c_type, 
           nSqlType, 
           pparm->pm_precision, 
           pparm->pm_scale, 
           pparm->pm_conv_data, 
           pparm->pm_conv_el_size,
           pparm->pm_conv_pInd));
    }

  return retcode;
}


static SQLRETURN
_SQLExecute_ConvParams (SQLHSTMT hstmt, BOOL bOutput)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;
  PPARM pparm;
  int maxpar;
  int i;
  SQLULEN j;
  SQLULEN cRows = pstmt->paramset_size;
  DM_CONV *conv = penv->conv;
  SQLRETURN retcode = SQL_SUCCESS;
  IODBC_CHARSET m_charset = CP_DEF;
  IODBC_CHARSET d_charset = CP_DEF;
  BOOL needRebind = FALSE;
  SQLLEN sz_mult = 1;
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;

  if (cRows == 0)
    cRows = 1;

  if (conv)
    {
      m_charset = conv ? conv->dm_cp: CP_DEF;
      d_charset = conv ? conv->drv_cp: CP_DEF;

      if (m_charset==CP_UTF16 && d_charset==CP_UCS4)
        sz_mult = 2;
      else if (m_charset==CP_UTF8 && d_charset==CP_UCS4)
        sz_mult = 4;
      else if (m_charset==CP_UTF8 && d_charset==CP_UTF16)
        sz_mult = 2;
      else
        sz_mult = 1;
    }
  
  maxpar = pstmt->st_nparam;

  if (penv->unicode_driver && !bOutput)
    {
      if (conv==NULL || (conv && conv->dm_cp == conv->drv_cp))
        {
          needRebind = FALSE;
        }
      else if ((m_charset==CP_UCS4 && d_charset==CP_UTF16)
             ||(m_charset==CP_UTF16 && d_charset==CP_UCS4)
             ||(m_charset==CP_UTF8 && d_charset==CP_UTF16)
             ||(m_charset==CP_UTF8 && d_charset==CP_UCS4))
        {
          /* check if we need rebind params */
          pparm = pstmt->st_pparam;
          for (i = 0; i < maxpar; i++, pparm++)
            {
              if (pparm->pm_data == NULL)
                continue;

              if (pparm->pm_c_type_orig == SQL_C_WCHAR)
                {
                  needRebind = TRUE;
                  break;
                }

            }
        }
    }

  if (needRebind && !bOutput)  /* mode this flag to PSTMT */
    {
      SQLULEN new_size = 0;
      void *buf = NULL;

      if (pstmt->param_bind_type) /* row-wise binding */
        {
          pparm = pstmt->st_pparam;
          for (i = 0; i < maxpar; i++, pparm++)
            {
              if (pparm->pm_data == NULL)
                continue;

              new_size += sizeof(SQLLEN);

              pparm->pm_conv_el_size = GetElementSize(pparm, conv);
              if (pparm->pm_c_type == SQL_C_WCHAR)
                pparm->pm_conv_el_size *= sz_mult;

              new_size += pparm->pm_conv_el_size;
            }
          
          if (pstmt->params_buf)
            {
              free(pstmt->params_buf);
              pstmt->params_buf = NULL;
            }

          buf = calloc((new_size*cRows), sizeof(char));
          if (!buf)
            {
              PUSHSQLERR (pstmt->herr, en_HY001);
              return SQL_ERROR;
            }
          pstmt->params_buf = buf;
          pstmt->conv_param_bind_type = new_size;

          /***** Reset Params *****/
          hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);
          if (hproc2 == SQL_NULL_HPROC)
	    {
	      PUSHSQLERR (pstmt->herr, en_IM001);
	      return SQL_ERROR;
	    }

          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	      (pstmt->dhstmt, (SQLUSMALLINT)SQL_RESET_PARAMS));
          if (!SQL_SUCCEEDED (retcode))
            return retcode;


          /***** Set Bind_type in driver to new size *****/
          if (dodbc_ver == SQL_OV_ODBC3)
            {
              CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc3, 
                penv->unicode_driver, en_SetStmtAttr, (pstmt->dhstmt, 
		(SQLINTEGER)SQL_ATTR_PARAM_BIND_TYPE, 
                (SQLPOINTER)new_size, 0));
              if (hproc3 == SQL_NULL_HPROC)
                {
	          PUSHSQLERR (pstmt->herr, en_IM001);
	          return SQL_ERROR;
                }
            }
          else
            {
              hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOption);
              if (hproc2 == SQL_NULL_HPROC)
                hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOptionA);

              if (hproc2 == SQL_NULL_HPROC)
                {
	          PUSHSQLERR (pstmt->herr, en_IM001);
	          return SQL_ERROR;
                }

              CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	          (pstmt->dhstmt, 
	           (SQLUSMALLINT)SQL_ATTR_PARAM_BIND_TYPE, 
	           (SQLUINTEGER)new_size));

            }
          if (!SQL_SUCCEEDED (retcode))
            return retcode;


          if (cRows > 1)
          {
            /***** Set ParamSet size *****/
            if (dodbc_ver == SQL_OV_ODBC3)
            {
              CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc3, 
                penv->unicode_driver, en_SetStmtAttr, (pstmt->dhstmt, 
		(SQLINTEGER)SQL_ATTR_PARAMSET_SIZE, 
                (SQLPOINTER)cRows, 0));
              if (hproc3 == SQL_NULL_HPROC)
                {
	          PUSHSQLERR (pstmt->herr, en_IM001);
	          return SQL_ERROR;
                }
            }
            if (!SQL_SUCCEEDED (retcode))
              return retcode;
          }


          /* rebind parameters */
          pparm = pstmt->st_pparam;
          buf = pstmt->params_buf;
          for (i = 0; i < maxpar; i++, pparm++)
            {
              if (pparm->pm_data == NULL)
                continue;

              pparm->pm_conv_data = buf;
              buf += pparm->pm_conv_el_size;

              pparm->pm_conv_pInd = buf;
              buf += sizeof(SQLLEN);

              retcode = _ReBindParam(pstmt, pparm);
              if (!SQL_SUCCEEDED (retcode))
                return retcode;
              pparm->rebinded = TRUE;
            
            }
        }

      else /* col-wise binding  rebind all for avoid OFFSET related issues */
        {

          pparm = pstmt->st_pparam;
          for (i = 0; i < maxpar; i++, pparm++)
            {
              SQLLEN *pInd_dm = NULL;
              SQLUINTEGER bindOffset = pstmt->param_bind_offset;

              if (pparm->pm_pInd)
                pInd_dm = ((SQLLEN*)((char*)pparm->pm_pInd + bindOffset));

              if (pparm->pm_data == NULL)
                continue;

              if (pparm->pm_tmp)
                {
                  free(pparm->pm_tmp);
                  pparm->pm_tmp = NULL;
                }
              if (pparm->pm_tmp_Ind)
                {
                  free(pparm->pm_tmp_Ind);
                  pparm->pm_tmp_Ind = NULL;
                }

              pparm->pm_conv_el_size = GetElementSize (pparm, conv);
              
              if (cRows == 1 && pInd_dm 
                  && (*pInd_dm == SQL_DATA_AT_EXEC 
                      || *pInd_dm <= SQL_LEN_DATA_AT_EXEC_OFFSET))
                {
                  pparm->pm_conv_pInd = pparm->pm_pInd;
                  pparm->pm_conv_data = pparm->pm_data;
                }
              else
                {
                  if (pparm->pm_c_type == SQL_C_WCHAR)
                    pparm->pm_conv_el_size *= sz_mult;

                  new_size = cRows * pparm->pm_conv_el_size;
                  buf = calloc(new_size, sizeof(char));
                  if (!buf)
                    {
                      PUSHSQLERR (pstmt->herr, en_HY001);
                      return SQL_ERROR;
                    }
                  pparm->pm_tmp = pparm->pm_conv_data = buf;

                  buf = calloc(cRows, sizeof(SQLLEN));
                  if (!buf)
                    {
                      PUSHSQLERR (pstmt->herr, en_HY001);
                      return SQL_ERROR;
                    }
                  pparm->pm_tmp_Ind = pparm->pm_conv_pInd = buf;
                }

              retcode = _ReBindParam(pstmt, pparm);
              if (!SQL_SUCCEEDED (retcode))
                return retcode;
              pparm->rebinded = TRUE;

            }

        }

      /***** Set ParamSet offset *****/
      if (dodbc_ver == SQL_OV_ODBC3)
        {
          CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc3, 
            penv->unicode_driver, en_SetStmtAttr, (pstmt->dhstmt, 
	    (SQLINTEGER)SQL_ATTR_PARAM_BIND_OFFSET_PTR, 
            (SQLPOINTER)0, 0));
          if (hproc3 == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (pstmt->herr, en_IM001);
	      return SQL_ERROR;
            }
        }
    }
  
  pparm = pstmt->st_pparam;
  for (i = 0; i < maxpar; i++, pparm++)
    {
      if (pparm->pm_data == NULL)
        continue;

      if (pparm->rebinded)
        {
          if (bOutput && pparm->pm_usage == SQL_PARAM_INPUT)
            continue;
          for (j = 0; j < cRows; j++)
            _ConvRebindedParam(pstmt, pparm, j, bOutput, conv);
        }
      else
        {
          if (bOutput && (pparm->pm_usage == SQL_PARAM_OUTPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
            {
              if (pparm->pm_c_type_orig != SQL_C_WCHAR)
                continue;

              for (j = 0; j < cRows; j++)
                _ConvParam(pstmt, pparm, j, bOutput, conv, penv->unicode_driver);
            }
          else if (!bOutput && (pparm->pm_usage == SQL_PARAM_INPUT || pparm->pm_usage == SQL_PARAM_INPUT_OUTPUT))
            {
              if (pparm->pm_c_type != SQL_C_WCHAR)
                continue;

              for (j = 0; j < cRows; j++)
                _ConvParam(pstmt, pparm, j, bOutput, conv, penv->unicode_driver);

              pparm->pm_c_type = SQL_C_CHAR;
            }
        }
    } /* next column */

  return SQL_SUCCESS;

}

#endif

static SQLRETURN
SQLExecute_Internal (SQLHSTMT hstmt)
{
  STMT (pstmt, hstmt);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode, rc;
  sqlstcode_t sqlstat = en_00000;

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	  sqlstat = en_S1010;
	  break;

	case en_stmt_executed_with_info:
	case en_stmt_executed:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  break;

	case en_stmt_cursoropen:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  break;

	case en_stmt_fetched:
	case en_stmt_xfetched:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  else
	    {
	      sqlstat = en_24000;
	    }
	  break;

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_Execute)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat == en_00000)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Execute);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  if ((rc = _SQLExecute_ConvParams(hstmt, FALSE)) != SQL_SUCCESS)
    return rc;
  
  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, (pstmt->dhstmt));

  /* stmt state transition */
  if (pstmt->asyn_on == en_Execute)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      rc = _SQLExecute_ConvParams(hstmt, TRUE);
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        retcode = rc;
    }

  switch (pstmt->state)
    {
    case en_stmt_prepared:
      switch (retcode)
	{
	case SQL_SUCCESS:
	  _iodbcdm_do_cursoropen (pstmt);
	  break;

	case SQL_SUCCESS_WITH_INFO:
	  pstmt->state = en_stmt_executed_with_info;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_Execute;
          pstmt->st_need_param = NULL;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_Execute;
	  break;

	default:
	  break;
	}
      break;

    case en_stmt_executed:
      switch (retcode)
	{
	case SQL_ERROR:
	  pstmt->state = en_stmt_prepared;
	  pstmt->cursor_state = en_stmt_cursor_no;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_Execute;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_Execute;
	  break;

	default:
	  break;
	}
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLExecute (SQLHSTMT hstmt)
{
  ENTER_STMT (hstmt,
    trace_SQLExecute (TRACE_ENTER, hstmt));

  retcode = SQLExecute_Internal (hstmt);

  LEAVE_STMT (hstmt,
    trace_SQLExecute (TRACE_LEAVE, hstmt));
}


SQLRETURN SQL_API
SQLExecDirect_Internal (SQLHSTMT hstmt,
    SQLPOINTER szSqlStr,
    SQLINTEGER cbSqlStr,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  SQLRETURN rc = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _SqlStr = NULL;
  CONV_DIRECT conv_direct = CD_NONE; 
  DM_CONV *conv = penv->conv;

  /* check arguments */
  if (szSqlStr == NULL)
    {
      sqlstat = en_S1009;
    }
  else if (cbSqlStr < 0 && cbSqlStr != SQL_NTS)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_fetched:
	case en_stmt_xfetched:
	  sqlstat = en_24000;
	  break;

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_ExecDirect)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  if (penv->unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!penv->unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct != CD_NONE)
    {
      _SqlStr = _iodbcdm_conv_var (pstmt, 0, szSqlStr, cbSqlStr, conv_direct);
      szSqlStr = _SqlStr;
      cbSqlStr = SQL_NTS;
    }

  if ((rc = _SQLExecute_ConvParams(hstmt, FALSE)) != SQL_SUCCESS)
    return rc;

  CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
    en_ExecDirect, (
       pstmt->dhstmt,
       szSqlStr,
       cbSqlStr));

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_FreeStmtVars(pstmt);
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }
    
  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtVars(pstmt);

  /* stmt state transition */
  if (pstmt->asyn_on == en_ExecDirect)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      rc = _SQLExecute_ConvParams(hstmt, TRUE);
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        retcode = rc;
    }

  if (pstmt->state <= en_stmt_executed)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	  _iodbcdm_do_cursoropen (pstmt);
	  pstmt->prep_state = 1;
	  break;

	case SQL_SUCCESS_WITH_INFO:
	  pstmt->state = en_stmt_executed_with_info;
	  pstmt->prep_state = 1;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_ExecDirect;
          pstmt->st_need_param = NULL;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_ExecDirect;
	  break;

	case SQL_ERROR:
	  pstmt->state = en_stmt_allocated;
	  pstmt->cursor_state = en_stmt_cursor_no;
	  pstmt->prep_state = 0;
	  break;

	default:
	  break;
	}
    }

  return retcode;
}


SQLRETURN SQL_API
SQLExecDirect (SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  ENTER_STMT (hstmt,
    trace_SQLExecDirect (TRACE_ENTER, hstmt, szSqlStr, cbSqlStr));

  retcode = SQLExecDirect_Internal(hstmt, szSqlStr, cbSqlStr, 'A');

  LEAVE_STMT (hstmt,
    trace_SQLExecDirect (TRACE_LEAVE, hstmt, szSqlStr, cbSqlStr));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLExecDirectA (SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  ENTER_STMT (hstmt,
    trace_SQLExecDirect (TRACE_ENTER, hstmt, szSqlStr, cbSqlStr));

  retcode = SQLExecDirect_Internal(hstmt, szSqlStr, cbSqlStr, 'A');

  LEAVE_STMT (hstmt,
    trace_SQLExecDirect (TRACE_LEAVE, hstmt, szSqlStr, cbSqlStr));
}


SQLRETURN SQL_API
SQLExecDirectW (SQLHSTMT hstmt, SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  ENTER_STMT (hstmt,
    trace_SQLExecDirectW (TRACE_ENTER, hstmt, szSqlStr, cbSqlStr));

  retcode = SQLExecDirect_Internal(hstmt, szSqlStr, cbSqlStr, 'W');

  LEAVE_STMT (hstmt,
    trace_SQLExecDirectW (TRACE_LEAVE, hstmt, szSqlStr, cbSqlStr));
}
#endif

static SQLRETURN
SQLMoreResults_Internal (SQLHSTMT hstmt)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;
  SQLRETURN rc;

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
#if 0
	case en_stmt_allocated:
	case en_stmt_prepared:
	  return SQL_NO_DATA_FOUND;
#endif

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  return SQL_ERROR;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_MoreResults)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_MoreResults);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc,
      (pstmt->dhstmt));

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO || retcode == SQL_NO_DATA_FOUND)
    {
      rc = _SQLExecute_ConvParams(hstmt, TRUE);
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        retcode = rc;
    }

  /* state transition */
  if (pstmt->asyn_on == en_MoreResults)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NO_DATA_FOUND:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  switch (pstmt->state)
    {
    case en_stmt_allocated:
    case en_stmt_prepared:
      /* driver should return SQL_NO_DATA_FOUND */
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_cursoropen;
	    }
	  else
	    {
	      pstmt->state = en_stmt_prepared;
	    }
      break;

    case en_stmt_executed_with_info:
    	_iodbcdm_do_cursoropen (pstmt);
	/* FALL THROUGH */

    case en_stmt_executed:
      if (retcode == SQL_NO_DATA_FOUND)
	{
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	    }
	  else
	    {
	      pstmt->state = en_stmt_cursoropen;
	    }
	}
      else if (retcode == SQL_STILL_EXECUTING)
	{
	  pstmt->asyn_on = en_MoreResults;
	}
      break;

    case en_stmt_cursoropen:
    case en_stmt_fetched:
    case en_stmt_xfetched:
      if (retcode == SQL_SUCCESS)
	{
	  break;
	}
      else if (retcode == SQL_NO_DATA_FOUND)
	{
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	    }
	  else
	    {
	      pstmt->state = en_stmt_allocated;
	    }
	}
      else if (retcode == SQL_STILL_EXECUTING)
	{
	  pstmt->asyn_on = en_MoreResults;
	}
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLMoreResults (SQLHSTMT hstmt)
{
  ENTER_STMT (hstmt,
    trace_SQLMoreResults (TRACE_ENTER, hstmt));

  retcode = SQLMoreResults_Internal (hstmt);

  LEAVE_STMT (hstmt,
    trace_SQLMoreResults (TRACE_LEAVE, hstmt));
}


static SQLRETURN
SQLPutData_Internal (
  SQLHSTMT		  hstmt,
  SQLPOINTER		  rgbValue, 
  SQLLEN		  cbValue)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc;
  SQLRETURN retcode;
  DM_CONV *conv = penv->conv;
  IODBC_CHARSET m_charset = CP_DEF;
  IODBC_CHARSET d_charset = CP_DEF;

  /* check argument value */
  if (rgbValue == NULL &&
      (cbValue != SQL_DEFAULT_PARAM && cbValue != SQL_NULL_DATA))
    {
      PUSHSQLERR (pstmt->herr, en_S1009);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state <= en_stmt_xfetched)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  else if (pstmt->asyn_on != en_PutData)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_PutData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  if (conv)
    {
      m_charset = conv ? conv->dm_cp: CP_DEF;
      d_charset = conv ? conv->drv_cp: CP_DEF;
    }

  if (pstmt->st_need_param != NULL && m_charset != d_charset
     &&  pstmt->st_need_param->pm_c_type_orig == SQL_C_WCHAR)
    {
      SQLLEN drv_cbValue;
      SQLPOINTER drv_rgbValue = conv_text_m2d_W2W(conv, rgbValue, cbValue,
                                  &drv_cbValue); 

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
          (pstmt->dhstmt, drv_rgbValue, drv_cbValue));

      if (drv_rgbValue != NULL)
        free(drv_rgbValue);
    }
  else
    {
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
          (pstmt->dhstmt, rgbValue, cbValue));
    }



  /* state transition */
  if (pstmt->asyn_on == en_PutData)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  /* must in mustput or canput states */
  switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      pstmt->state = en_stmt_canput;
      break;

    case SQL_ERROR:
      switch (pstmt->need_on)
	{
	case en_ExecDirect:
	  pstmt->state = en_stmt_allocated;
	  pstmt->need_on = en_NullProc;
	  break;

	case en_Execute:
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	      pstmt->need_on = en_NullProc;
	    }
	  break;

	case en_SetPos:
	  /* Is this possible ???? */
	  pstmt->state = en_stmt_xfetched;
	  break;

	default:
	  break;
	}
      break;

    case SQL_STILL_EXECUTING:
      pstmt->asyn_on = en_PutData;
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLPutData (
  SQLHSTMT		  hstmt, 
  SQLPOINTER		  rgbValue, 
  SQLLEN		  cbValue)
{
  ENTER_STMT (hstmt,
    trace_SQLPutData (TRACE_ENTER, hstmt, rgbValue, cbValue));

  retcode = SQLPutData_Internal (hstmt, rgbValue, cbValue);

  LEAVE_STMT (hstmt,
    trace_SQLPutData (TRACE_LEAVE, hstmt, rgbValue, cbValue));
}


static SQLRETURN
SQLParamData_Internal (SQLHSTMT hstmt, SQLPOINTER * prgbValue)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  /* check argument */

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state <= en_stmt_xfetched)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  else if (pstmt->asyn_on != en_ParamData)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ParamData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, (pstmt->dhstmt, prgbValue));

  /* state transition */
  if (pstmt->asyn_on == en_ParamData)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  if (pstmt->state < en_stmt_needdata)
    {
      return retcode;
    }

  switch (retcode)
    {
    case SQL_ERROR:
      switch (pstmt->need_on)
	{
	case en_ExecDirect:
	  pstmt->state = en_stmt_allocated;
	  break;

	case en_Execute:
	  pstmt->state = en_stmt_prepared;
	  break;

	case en_SetPos:
	  pstmt->state = en_stmt_xfetched;
	  pstmt->cursor_state = en_stmt_cursor_xfetched;
	  break;

	default:
	  break;
	}
      pstmt->need_on = en_NullProc;
      break;

    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      switch (pstmt->state)
	{
	case en_stmt_needdata:
	  pstmt->state = en_stmt_mustput;
	  break;

	case en_stmt_canput:
	  switch (pstmt->need_on)
	    {
	    case en_SetPos:
	      pstmt->state = en_stmt_xfetched;
	      pstmt->cursor_state = en_stmt_cursor_xfetched;
	      break;

	    case en_ExecDirect:
	    case en_Execute:
	      _iodbcdm_do_cursoropen (pstmt);
	      break;

	    default:
	      break;
	    }
	  break;

	default:
	  break;
	}
      pstmt->need_on = en_NullProc;
      break;

    case SQL_NEED_DATA:
      pstmt->state = en_stmt_mustput;
      pstmt->st_need_param = NULL;

      PPARM pparm = pstmt->st_pparam;
      for (int i = 0; i < pstmt->st_nparam; i++, pparm++)
        {
          if (pparm->pm_data == NULL)
            continue;

          if ((pparm->pm_c_type_orig == SQL_C_WCHAR 
               || pparm->pm_c_type_orig == SQL_C_CHAR 
               || pparm->pm_c_type_orig == SQL_C_BINARY)
              && prgbValue != NULL && pparm->pm_data == *prgbValue)
            {
              pstmt->st_need_param = pparm;
              break;
            }
        }
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLParamData (SQLHSTMT hstmt, SQLPOINTER * prgbValue)
{
  ENTER_STMT (hstmt,
    trace_SQLParamData (TRACE_ENTER, hstmt, prgbValue));

  retcode = SQLParamData_Internal (hstmt, prgbValue);

  LEAVE_STMT (hstmt,
    trace_SQLParamData (TRACE_LEAVE, hstmt, prgbValue));
}


static SQLRETURN
SQLNumParams_Internal (SQLHSTMT hstmt, SQLSMALLINT * pcpar)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  /* check argument */
  if (!pcpar)
    {
      return SQL_SUCCESS;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  return SQL_ERROR;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_NumParams)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_NumParams);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, (pstmt->dhstmt, pcpar));

  /* state transition */
  if (pstmt->asyn_on == en_NumParams)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  break;

	default:
	  return retcode;
	}
    }

  if (retcode == SQL_STILL_EXECUTING)
    {
      pstmt->asyn_on = en_NumParams;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLNumParams (SQLHSTMT hstmt, SQLSMALLINT * pcpar)
{
  ENTER_STMT (hstmt,
    trace_SQLNumParams (TRACE_ENTER, hstmt, pcpar));

  retcode = SQLNumParams_Internal (hstmt, pcpar);

  LEAVE_STMT (hstmt,
    trace_SQLNumParams (TRACE_LEAVE, hstmt, pcpar));
}


static SQLRETURN
SQLDescribeParam_Internal (
    SQLHSTMT		  hstmt,
    SQLUSMALLINT	  ipar,
    SQLSMALLINT		* pfSqlType,
    SQLULEN		* pcbColDef,
    SQLSMALLINT		* pibScale, 
    SQLSMALLINT 	* pfNullable)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  GENV (genv, pdbc->genv);

  HPROC hproc;
  SQLRETURN retcode;

  /* check argument */
  if (ipar == 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1093);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  return SQL_ERROR;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_DescribeParam)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_DescribeParam);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc,
      (pstmt->dhstmt, ipar, pfSqlType, pcbColDef, pibScale, pfNullable));

  /*
   *  Convert sql type to ODBC version of application
   */
  if (SQL_SUCCEEDED(retcode) && pfSqlType)
    *pfSqlType = _iodbcdm_map_sql_type (*pfSqlType, genv->odbc_ver);


  /* state transition */
  if (pstmt->asyn_on == en_DescribeParam)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  break;

	default:
	  return retcode;
	}
    }

  if (retcode == SQL_STILL_EXECUTING)
    {
      pstmt->asyn_on = en_DescribeParam;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLDescribeParam (
  SQLHSTMT		  hstmt,
  SQLUSMALLINT		  ipar,
  SQLSMALLINT 		* pfSqlType,
  SQLULEN 		* pcbColDef,
  SQLSMALLINT 		* pibScale,
  SQLSMALLINT 		* pfNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLDescribeParam (TRACE_ENTER,
      hstmt, ipar, pfSqlType,
      pcbColDef, pibScale, pfNullable));

  retcode = SQLDescribeParam_Internal ( hstmt, ipar, pfSqlType,
      pcbColDef, pibScale, pfNullable);

  LEAVE_STMT (hstmt,
    trace_SQLDescribeParam (TRACE_LEAVE,
      hstmt, ipar, pfSqlType,
      pcbColDef, pibScale, pfNullable));
}
