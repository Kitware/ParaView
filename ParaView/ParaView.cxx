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
#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "vtkPVSlave.h"
#include "vtkPVApplication.h"
#include "vtkTclUtil.h"


extern "C" {void vtkPVSlaveStart(vtkMultiProcessController *controller);}

// external global variable.
vtkMultiProcessController *VTK_PV_UI_CONTROLLER = NULL;


struct vtkPVArgs
{
  int argc;
  char **argv;
};



// Each process starts with this method.  One process is designated as "master" 
// and starts the application.  The other processes are slaves to the application.
void Process_Init(vtkMultiProcessController *controller, void *arg )
{
  vtkPVArgs *pvArgs = (vtkPVArgs *)arg;
  int myId, numProcs;
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  
  sleep(myId);
  
  if (myId == 0)
    { // The last process is for UI.
    // We need to pass the local controller to the UI process.
    putenv("DISPLAY=:6.0");
    //putenv("DISPLAY=:0.0");
    Tcl_Interp *interp = vtkPVApplication::InitializeTcl(pvArgs->argc,pvArgs->argv);
    vtkPVApplication *app = vtkPVApplication::New();
    app->SetController(controller);
    app->Script("wm withdraw .");
    
    app->Start(pvArgs->argc,pvArgs->argv);
    app->Delete();
    }
  else
    {
    // The last is used as a master, so reduce the slave count by one.
    vtkPVSlaveStart(controller);
    }
}

#ifdef _WIN32
#include <windows.h>



int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd)
{
  int argc;
  char *argv[5];

  argv[0] = new char [strlen(lpCmdLine)+1];
  argv[1] = new char [strlen(lpCmdLine)+1];
  argv[2] = new char [strlen(lpCmdLine)+1];
  argv[3] = new char [strlen(lpCmdLine)+1];
  argv[4] = new char [strlen(lpCmdLine)+1];

  unsigned int i;
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

  vtkMultiProcessController *controller = vtkMultiProcessController::New();
  controller->SetNumberOfProcesses(1);
  controller->Initialize(argc, argv);


  Tcl_Interp *interp = vtkPVApplication::InitializeTcl(pvArgs->argc,pvArgs->argv);
  vtkPVApplication *app = vtkPVApplication::New();
  app->SetController(controller);
  app->Script("wm withdraw .");
    
  app->Start(pvArgs->argc,pvArgs->argv);
  app->Delete();
  
  controller->Delete();
  
  delete [] argv[0];
  delete [] argv[1];
  delete [] argv[2];
  delete [] argv[3];
  delete [] argv[4];
  return 0;
}
#else
int main(int argc, char *argv[])
{
  // New processes need these args to initialize.
  vtkPVArgs pvArgs;
  pvArgs.argc = argc;
  pvArgs.argv = argv;

  vtkMultiProcessController *controller = vtkMultiProcessController::New();  
  controller->Initialize(argc, argv);
  //controller->SetNumberOfProcesses(2);
  controller->SetSingleMethod(Process_Init, (void *)(&pvArgs));
  controller->SingleMethodExecute();

  //Process_Init((void *)(&pvArgs));
  
  controller->Delete();
  
  return 0;
}
#endif






