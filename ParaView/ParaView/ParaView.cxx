/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParaView.cxx
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
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include "vtkMultiProcessController.h"
#include "vtkPVMPIProcessModule.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVApplication.h"

#include "vtkObject.h"
#include "vtkTclUtil.h"

//----------------------------------------------------------------------------
int MyMain(int argc, char *argv[])
{
  int retVal = 0;
  int myId = 0;
  vtkPVProcessModule *pm;
  vtkPVApplication *app;

#ifdef VTK_USE_MPI
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);
  // Might as well get our process ID here.  I use it to determine
  // Whether to initialize tk.  Once again, splitting Tk and Tcl 
  // initialization would clean things up.
  MPI_Comm_rank(MPI_COMM_WORLD,&myId); 
#endif

  // The server is a special case.  We do not initialize Tk for process 0.
  // I would rather have application find this command line option, but
  // I cannot create an application before I initialize Tcl.
  // I could clean this up if I separate the initialization of Tk and Tcl.
  // I do not do this because it would affect other applications.
  int serverMode = 0;
  int idx;
  for (idx = 0; idx < argc; ++idx)
    {
    if (strcmp(argv[idx],"--server") == 0 || strcmp(argv[idx],"-v") == 0)
      {
      serverMode = 1;
      }
    }

  // Initialize Tcl/Tk.
  Tcl_Interp *interp;
  if (serverMode || myId > 0)
    { // DO not initialize Tk.
    vtkKWApplication::SetWidgetVisibility(0);
    }
  interp = vtkPVApplication::InitializeTcl(argc,argv);

  // Create the application to parse the command line arguments.
  app = vtkPVApplication::New();
  if (myId == 0 && app->ParseCommandLineArguments(argc, argv))
    {
    app->SetStartGUI(0);
    }

  // Create the process module for initializing the processes.
  if (app->GetClientMode() || app->GetServerMode())
    {
    vtkPVClientServerModule *processModule = vtkPVClientServerModule::New();
    pm = processModule;
    }
  else
    {
#ifdef VTK_USE_MPI
    vtkPVMPIProcessModule *processModule = vtkPVMPIProcessModule::New();
#else 
    vtkPVProcessModule *processModule = vtkPVProcessModule::New();
#endif
    pm = processModule;
    }

  pm->SetApplication(app);
  app->SetProcessModule(pm);

  retVal = pm->Start(argc, argv);

  // Clean up for exit.
  app->Delete();
  pm->Delete();
  pm = NULL;
  Tcl_DeleteInterp(interp);

  return retVal;
}






#ifdef _WIN32
#include <windows.h>

int __stdcall WinMain(HINSTANCE vtkNotUsed(hInstance), 
                      HINSTANCE vtkNotUsed(hPrevInstance),
                      LPSTR lpCmdLine, int vtkNotUsed(nShowCmd))
{
  int          argc;
  int          retVal;
  char**       argv;
  unsigned int i;
  int          j;

  // parse a few of the command line arguments
  // a space delimites an argument except when it is inside a quote

  argc = 1;
  int pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    }

  argv = (char**)malloc(sizeof(char*)* (argc+1));

  argv[0] = (char*)malloc(1024);
  ::GetModuleFileName(0, argv[0],1024);

  for(j=1; j<argc; j++)
    {
    argv[j] = (char*)malloc(strlen(lpCmdLine)+10);
    }
  argv[argc] = 0;

  argc = 1;
  pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    }
  argv[argc] = 0;

  // Initialize the processes and start the application.
  retVal = MyMain(argc, argv);

  // Delete arguments
  for(j=0; j<argc; j++)
    {
    free(argv[j]);
    }
  free(argv);

  // !!!!! Should this be in the common main. !!!!!!
  Tcl_Finalize();
  return retVal;;
}
#else
int main(int argc, char *argv[])
{
  return MyMain(argc, argv);
}
#endif
