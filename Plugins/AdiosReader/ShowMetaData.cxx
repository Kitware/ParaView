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
using namespace std;

int main( int argc, char *argv[] )
{
  if(argc == 1)
    {
    cout << "Please provide the Path to the Adios file to read as argument." << endl;
    return 1;
    }

  // Read file name.
  char* fname = argv[1];

  // Create the Adios file manager.
  AdiosFile* adiosFile = new AdiosFile(fname);
  adiosFile->PrintInfo();
  delete adiosFile;

  return EXIT_SUCCESS;
}
