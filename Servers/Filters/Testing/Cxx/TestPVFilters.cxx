/*=========================================================================

  Program:   ParaView
  Module:    TestPVFilters.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDReader.h"
#include "vtkUnstructuredGrid.h"

int main(int argc, char* argv[])
{
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vox8_bin.vtk");

  vtkPVDReader *reader = vtkPVDReader::New();
  reader->SetFileName( fname );
  reader->Update();


  delete [] fname;

  return 0;
}


