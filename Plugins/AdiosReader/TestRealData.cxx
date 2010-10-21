/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRealData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of TestRealData
// .SECTION Description
// Load all 3D variables with our VTK inner object management for the adios lib.

#include "vtkAdiosInternals.h"
#include <iostream>
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include <vtkstd/string>
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
    {
    //fname = "/home/seb/Downloads/pixie3d_3D.bp";
    cerr << "Provide as argument the Adios file to read." << endl;
    return EXIT_FAILURE;
    }

  // Create the Adios file manager.
  AdiosFile* adiosFile = new AdiosFile(fname);
  adiosFile->Open();
  //adiosFile->PrintInfo();

  AdiosVariableMapIterator iter = adiosFile->Variables.begin();
  int ts = -1;
  for(; iter != adiosFile->Variables.end() ; iter++)
    {
    vtkstd::string name = iter->second.Name;
    cout << name.c_str() << endl;
    if(name.find("_50/") != vtkstd::string::npos)
      {
      if(ts != 50)
        {
        adiosFile->GetPixieRectilinearGrid(50)->Delete();
        }
      ts = 50;
      }
    else if(name.find("_100/") != vtkstd::string::npos)
      {
      if(ts != 100)
        {
        adiosFile->GetPixieRectilinearGrid(100)->Delete();
        }

      ts = 100;
      }
    else
      {
      if(ts != 0)
        {
        adiosFile->GetPixieRectilinearGrid(0)->Delete();
        }
      ts = 0;
      }
    vtkDataArray* array = adiosFile->ReadVariable(name.c_str());
    if(array)
      {
      array->Delete();
      cout << "delete ok" << endl;
      }
    }
  delete adiosFile;

  return EXIT_SUCCESS;
}
