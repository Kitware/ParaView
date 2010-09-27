/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRealDataAdiosLib.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of TestRealDataAdiosLib
// .SECTION Description
// Load all 3D variables and print error message.

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
    {
    //fname = "/home/seb/Downloads/pixie3d_3D.bp";
    cerr << "Provide as argument the Adios file to read." << endl;
    return EXIT_FAILURE;
    }

  int returnValue = -1;
  ADIOS_FILE* file = adios_fopen(fname, 0);
  ADIOS_GROUP* group = adios_gopen_byid(file, 0);
  for(int varIdx = 0; varIdx < group->vars_count ; varIdx++)
    {
    cout << "============ " << group->var_namelist[varIdx] << " ===========" << endl;

    ADIOS_VARINFO *varInfo = adios_inq_var_byid(group, varIdx);
    if(varInfo->ndim < 3)
      {
      adios_free_varinfo(varInfo);
      continue;
      }

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

    void* array = new unsigned char[arraySize];
    returnValue = adios_read_var_byid(group,
                                      varInfo->varid,
                                      start,
                                      count,
                                      array);

    cout << "void* Adios read var say " << returnValue << endl;
    cout << adios_errmsg() << endl;

    adios_free_varinfo(varInfo);
    free(array);
    }
  adios_gclose(group);
  adios_fclose(file);

  return EXIT_SUCCESS;
}
