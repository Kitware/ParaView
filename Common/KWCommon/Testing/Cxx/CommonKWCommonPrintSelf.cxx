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
#include "vtkKWIcon.h"
#include "vtkKWProcessStatistics.h"
#include "vtkKWRegisteryUtilities.h"
#include "vtkKWRemoteExecute.h"
#include "vtkKWSerializer.h"
#include "vtkString.h"

#ifdef _WIN32
# include "vtkKWWin32RegisteryUtilities.h"
#else
# include "vtkKWUNIXRegisteryUtilities.h"
#endif

int main(int , char* [])
{
  vtkObject *c;
  c = vtkKWArguments::New(); c->Print( cout ); c->Delete();
  c = vtkKWIcon::New(); c->Print( cout ); c->Delete();
  c = vtkKWProcessStatistics::New(); c->Print( cout ); c->Delete();
  c = vtkKWRegisteryUtilities::New(); c->Print( cout ); c->Delete();
  c = vtkKWRemoteExecute::New(); c->Print( cout ); c->Delete();
  c = vtkKWSerializer::New(); c->Print( cout ); c->Delete();
#ifdef _WIN32
  c = vtkKWWin32RegisteryUtilities::New(); c->Print( cout ); c->Delete();
#else
  c = vtkKWUNIXRegisteryUtilities::New(); c->Print( cout ); c->Delete();
#endif
  c = vtkString::New(); c->Print( cout ); c->Delete();

  return 0;
}
