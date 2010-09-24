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
#include "vtkDataArray.h"
#include <iostream>
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

  int returnValue = -1;
  const char* varName[6] = {"/Timestep_0/nodes/X",
                            "/Timestep_0/Car_variables/Bx",
                            "/Timestep_50/nodes/X",
                            "/Timestep_50/Car_variables/Bx",
                            "/Timestep_100/nodes/X",
                            "/Timestep_100/Car_variables/Bx"};
  for(int varIndex = 0; varIndex < 6 ; varIndex++)
    {
    cout << "============ " << varName[varIndex] << " ===========" << endl;
    ADIOS_FILE* file = adios_fopen(fname, 0);
    ADIOS_GROUP* group = adios_gopen_byid(file, 0);
    ADIOS_VARINFO *varInfo = adios_inq_var(group, varName[varIndex]);

    // Read data here...
    uint64_t start[4] = {0,0,0,0};
    uint64_t count[4] = { varInfo->dims[0],
                          varInfo->dims[1],
                          varInfo->dims[2],
                          0 };

    uint64_t nbElements = count[0] * count[1] * count[2];
    size_t arraySize = nbElements * adios_type_size(varInfo->type, NULL);
    cout << "nb elements: " << nbElements << " total size: " << arraySize << endl;
    cout << "Time idx: " << varInfo->timedim << endl;
    cout << "varIdx: " << varInfo->varid << endl;
    cout << "var ndim: " << varInfo->ndim << endl;
    cout << "start: " << start[0] << " " << start[1] << " " << start[2] << " " << start[3] << endl;
    cout << "count: " << count[0] << " " << count[1] << " " << count[2] << " " << count[3] << endl;

//    void* array = malloc(arraySize);
//    int returnValue = adios_read_var_byid(group,
//                                          varInfo->varid,
//                                          start,
//                                          count,
//                                          array);

//    cout << "void* Adios read var say " << returnValue << endl;
//    cout << adios_errmsg() << endl;


    double* arrayDouble = new double[nbElements + 1];
    returnValue = adios_read_var_byid(group,
                                      varInfo->varid,
                                      start,
                                      count,
                                      arrayDouble);

    cout << "double* Adios read var say " << returnValue << endl;
    cout << adios_errmsg() << endl;

//    vtkDoubleArray* arrayVTK = vtkDoubleArray::New();
//    arrayVTK->SetName("X");
//    arrayVTK->SetNumberOfComponents(1);
//    arrayVTK->SetNumberOfTuples(nbElements);
//    returnValue = adios_read_var_byid(group,
//                                      varInfo->varid,
//                                      start,
//                                      count,
//                                      arrayVTK->GetVoidPointer(0));

//    cout << "vtkDoubleArray: Adios read var say " << returnValue << endl;
//    cout << adios_errmsg() << endl;


    adios_free_varinfo(varInfo);
    adios_gclose(group);
    adios_fclose(file);


//    free(array);
    delete[] arrayDouble;
//    arrayVTK->Delete();

  // ===============================================================
//    AdiosFile* adiosFile = new AdiosFile(fname);
//    vtkDataArray* arrayVTK2 = adiosFile->ReadVariable(varName[varIndex], 0);
//    arrayVTK2->Delete();
//    delete adiosFile;
    }

  return EXIT_SUCCESS;
}
