/*=========================================================================

  Program:   ParaView
  Module:    TestLib.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWProcessStatistics.h"
#include "vtkKWRemoteExecute.h"

int main(int , char* [])
{
  vtkObject *c;
  c = vtkKWProcessStatistics::New(); c->Print( cout ); c->Delete();
  c = vtkKWRemoteExecute::New(); c->Print( cout ); c->Delete();

  return 0;
}
