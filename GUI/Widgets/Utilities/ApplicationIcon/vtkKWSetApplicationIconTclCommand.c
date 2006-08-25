/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <windows.h>
#include <winuser.h>
#include <winbase.h>

#include "vtkKWSetApplicationIconTclCommand.h"

//----------------------------------------------------------------------------

int vtkKWSetApplicationIconCmdInternal(Tcl_Interp *interp, 
                                       const char *app_name,
                                       int icon_res_id,
                                       int set_small)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  char cmd[1024];
  int error;
  HWND winHandle;
  DWORD app_path_length;
  char app_path[_MAX_PATH];
  HINSTANCE hInst = 0;
  HANDLE hIcon;
  LPVOID lpMsgBuf;

  // Get window handle (and convert it to Windows HWND)

  sprintf(cmd, "wm frame .");
  error = Tcl_GlobalEval(interp, cmd);
  if (error != TCL_OK)
    {
    return error;
    }

  sscanf(interp->result, "0x%x", (int*)&winHandle);

  // If the app name is empty, try to find the current application name

  if (!app_name || !*app_name)
    {
    app_path_length = GetModuleFileName(NULL, app_path, _MAX_PATH);
    if (app_path_length)
      {
      app_name = app_path;
      }
    }
                                   
  // Get application instance

  hInst = LoadLibrary(app_name);
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

  hIcon = LoadImage(hInst,
                    MAKEINTRESOURCE(icon_res_id),
                    IMAGE_ICON,
                    0,
                    0,
                    0);

  // Set icon

  if (hIcon != NULL)
    {
    SetClassLong(winHandle, set_small ? GCL_HICONSM : GCL_HICON, (LONG)hIcon);
    }
#if 0
  else
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
#endif

#endif // WIN32

  return TCL_OK;
}

//----------------------------------------------------------------------------
int vtkKWSetApplicationIcon(Tcl_Interp *interp, 
                            const char *app_name, 
                            int icon_res_id)
{
  return vtkKWSetApplicationIconCmdInternal(
    interp, app_name, icon_res_id, 0);
}

//----------------------------------------------------------------------------
int vtkKWSetApplicationSmallIcon(Tcl_Interp *interp, 
                                 const char *app_name, 
                                 int icon_res_id)
{
  return vtkKWSetApplicationIconCmdInternal(
    interp, app_name, icon_res_id, 1);
}

//----------------------------------------------------------------------------
int vtkKWSetApplicationIconCmd(ClientData clientdata, 
                               Tcl_Interp *interp, 
                               int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4 && TCL_RELEASE_LEVEL >= TCL_FINAL_RELEASE)
                               CONST84
#endif
                               char *argv[])
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  int error;
  int icon_res_id;
  int set_small;

  clientdata = 0; // To avoid warning: unreferenced formal parameter

  // Check usage

  if (argc < 3)
    {
    interp->result = 
      "Usage: vtkKWSetApplicationIcon app_name icon_res_id [small|big]";
    return TCL_ERROR;
    }

  // Get resource ID

  error = Tcl_GetInt(interp, argv[2], &icon_res_id);
  if (error != TCL_OK)
    {
    return error;
    }
  
  // Get small or big ?

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

  return vtkKWSetApplicationIconCmdInternal(
    interp, argv[1], icon_res_id, set_small);

#else

  (void)clientdata;
  (void)interp;
  (void)argc;
  (void)argv;

  return TCL_OK;

#endif
}

//----------------------------------------------------------------------------

int vtkKWSetApplicationIconTclCommand_DoInit(Tcl_Interp *interp)
{
  Tcl_CreateCommand(interp, 
                    "vtkKWSetApplicationIcon", 
                    vtkKWSetApplicationIconCmd,
                    (ClientData)NULL,
                    (Tcl_CmdDeleteProc*)NULL);
  return TCL_OK;
}


