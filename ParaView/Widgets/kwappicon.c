/*=========================================================================

  Program:   Visualization Toolkit
  Module:    kwappicon.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include <windows.h>
#include <winuser.h>
#include <winbase.h>

#include <kwappicon.h>

//----------------------------------------------------------------------------

int SetApplicationIconCmd(ClientData clientdata, 
                          Tcl_Interp *interp, 
                          int argc, char *argv[])
{
#ifdef _WIN32
  HWND winHandle;
  HANDLE hIcon;
  LPVOID lpMsgBuf;
  HINSTANCE hInst = 0;
  int iconID, error, set_small;
  char cmd[1024];

  clientdata = 0; // To avoid warning: unreferenced formal parameter

  // Check usage

  if (argc < 3)
    {
    interp->result = "Usage: SetApplicationIcon app_name icon_res_id [small|big]";
    return TCL_ERROR;
    }
  
  // Get window handle (and convert it to Windows HWND)

  sprintf(cmd, "wm frame .");
  error = Tcl_GlobalEval(interp, cmd);
  if (error != TCL_OK)
    {
    return error;
    }

  sscanf(interp->result, "0x%x", (int*)&winHandle);

  // Get application instance

  hInst = LoadLibrary(argv[1]);
  if (hInst == NULL)
    {
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, 
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf,
                  0,
                  NULL);
    sprintf(interp->result, "%s", (LPCTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
    return TCL_ERROR;
    }

  // Load icon from its resource ID

  error = Tcl_GetInt(interp, argv[2], &iconID);
  if (error != TCL_OK)
    {
    return error;
    }

  hIcon = LoadImage(hInst,
                    MAKEINTRESOURCE(iconID),
                    IMAGE_ICON,
                    0,
                    0,
                    0);
  if (hIcon == NULL)
    {
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf,
                  0,
                  NULL);
    sprintf(interp->result, "%s", (LPCTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
    return TCL_ERROR;
    }

  // Set icon

  set_small = 0;
  if (argc > 3)
    {
    if (!strcmp(argv[3], "small"))
      {
      set_small = 1;
      }
    else if (strcmp(argv[3], "big"))
      {
      sprintf(interp->result, "Error: %s (expecting 'big' or 'small')", 
              argv[3]);
      return TCL_ERROR;
      }
    }

  SetClassLong(winHandle, set_small ? GCL_HICONSM : GCL_HICON, (LPARAM)hIcon);

#endif // WIN32

  return TCL_OK;
}

//----------------------------------------------------------------------------

int ApplicationIcon_DoInit(Tcl_Interp *interp)
{
  Tcl_CreateCommand(interp, 
                    "SetApplicationIcon", 
                    SetApplicationIconCmd,
                    (ClientData)NULL,
                    (Tcl_CmdDeleteProc*)NULL);
  return TCL_OK;
}
