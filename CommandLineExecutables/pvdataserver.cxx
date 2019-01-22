/*=========================================================================

Program:   ParaView
Module:    pvdataserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pvserver_common.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Init current process type
  return RealMain(argc, argv, vtkProcessModule::PROCESS_DATA_SERVER) ? 0 : 1;
}
