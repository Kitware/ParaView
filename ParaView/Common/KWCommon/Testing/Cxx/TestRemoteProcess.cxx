/*=========================================================================

  Program:   ParaView
  Module:    TestRemoteProcess.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRemoteExecute.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include "vtkWindows.h"
#endif

int main()
{
  const char* args = "/usr/bin/find /usr -type f -name \\*andy\\*";
  vtkKWRemoteExecute* re = vtkKWRemoteExecute::New();
  re->SetRemoteHost("public");
  re->RunRemoteCommand(args);
  while( re->GetResult() == vtkKWRemoteExecute::RUNNING )
    {
    cout << "Waiting" << endl;
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
    }
  re->WaitToFinish();
  
  re->Delete();
  return 0;
}
