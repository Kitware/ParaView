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
#include "pvinit.h"
#include "vtkObject.h"
#include "vtkThreadedController.h"
#include "vtkMultiProcessController.h"


struct vtkPVArgs
{
  int argc;
  char **argv;
};

// Temporary replacement of vtkMultiThreader stuff
//typedef int VTK_THREAD_RETURN_TYPE;
//#define VTK_THREAD_RETURN_VALUE 0 

VTK_THREAD_RETURN_TYPE Process_Init( void *arg )
{
  vtkPVArgs *pvArgs = (vtkPVArgs *)arg;
  vtkObject::New();
  vtkMultiProcessController *controller;
  //int numProcs, myId;

  controller = vtkThreadedController::RegisterAndGetGlobalController(NULL);
  //numProcs = controller->GetNumberOfProcesses();
  //myId = controller->GetLocalProcessId();

  //vtkGenericWarningMacro("Process_Init: " << myId << " of " << numProcs);

  Et_Init(pvArgs->argc, pvArgs->argv);

  return VTK_THREAD_RETURN_VALUE;
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
  
  // New processes need these args to initialize.
  vtkPVArgs pvArgs;
  pvArgs.argc = argc;
  pvArgs.argv = argv;
  
  vtkThreadedController *controller = vtkThreadedController::New();
  controller->SetNumberOfProcesses(1);
  controller->Initialize(argc, argv);
  controller->SetSingleMethod(Process_Init, (void *)(&pvArgs));
  controller->SingleMethodExecute();

  //Process_Init((void *)(&pvArgs));

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
  
  //vtkThreadedController *controller = vtkThreadedController::New();
  //controller->SetNumberOfProcesses(1);
  //controller->Initialize(argc, argv);
  //controller->SetSingleMethod(Process_Init, (void *)(&pvArgs));
  //controller->SingleMethodExecute();

  Process_Init((void *)(&pvArgs));
  
  return 0;
}
#endif






