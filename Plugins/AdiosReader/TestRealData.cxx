/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ShowMetaData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of ShowMetaData
// .SECTION Description
// Print set of meta-data informations

#include "vtkAdiosInternals.h"
#include <iostream>
#include "vtkDataArray.h"
#include "vtkDataSet.h"
using namespace std;

int main( int argc, char *argv[] )
{
  char* fname = NULL;
  if(argc == 2)
    {
    fname = argv[1];
    }

 // Read file name.
  if(!fname)
    fname = "/home/seb/Downloads/pixie3d_3D.bp";

  // Create the Adios file manager.
  AdiosFile* adiosFile = new AdiosFile(fname);
  adiosFile->Open();
  //adiosFile->PrintInfo();

  const char* varName[6] = {"/Timestep_0/nodes/X",
                            "/Timestep_0/Car_variables/Bx",
                            "/Timestep_50/nodes/X",
                            "/Timestep_50/Car_variables/Bx",
                            "/Timestep_100/nodes/X",
                            "/Timestep_100/Car_variables/Bx"};
  int timestep[6] = {0,0,50,50,100,100};
  for(int varIndex = 0; varIndex < 6 ; varIndex++)
    {
    vtkDataArray* array = adiosFile->ReadVariable(varName[varIndex],timestep[varIndex]);
    if(array) array->Delete();
    }

  //vtkDataSet* ds = adiosFile->GetPixieStructuredGrid("/Timestep_0/Car_variables/By", 0);
  //if(ds) ds->Delete();
  delete adiosFile;

  return EXIT_SUCCESS;
}
