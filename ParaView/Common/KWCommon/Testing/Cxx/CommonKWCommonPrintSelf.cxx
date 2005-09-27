/*=========================================================================

  Program:   ParaView
  Module:    CommonKWCommonPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWArguments.h"
#include "vtkKWProcessStatistics.h"
#include "vtkKWRemoteExecute.h"
#include "vtkKWSerializer.h"

int main(int , char* [])
{
  vtkObject *c;
  c = vtkKWArguments::New(); c->Print( cout ); c->Delete();
  c = vtkKWProcessStatistics::New(); c->Print( cout ); c->Delete();
  c = vtkKWRemoteExecute::New(); c->Print( cout ); c->Delete();
  c = vtkKWSerializer::New(); c->Print( cout ); c->Delete();

  return 0;
}
