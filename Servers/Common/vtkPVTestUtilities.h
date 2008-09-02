/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTestUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTestUtilities - A class to facilitate common test operations
// .SECTION Description
// The class provides two main functions:
//
// 1) OS independent path name concatenation, specific to ctest 
//    temp directory and data root conventions used throughout
//    paraview.
// 2) Comparision of point data, point by point, tuple by tuple.
//

#ifndef vtkPVTestUtilities_h
#define vtkPVTestUtilities_h

#include "vtkstd/vector"//
#include "vtkType.h"//


class vtkPolyData;
class vtkDataArray;

class vtkPVTestUtilities
{
  public:
    // Description:
    // Initialize the object from command tail arguments.
    vtkPVTestUtilities(int argc, char **argv)
    {
      this->Argc=argc;
      this->Argv=argv;
      if (!((argc==0)||(argv==0)))
      {
        this->DataRoot=this->GetDataRoot();
        this->TempRoot=this->GetTempRoot();
      }
    }
    // Description:
    // Given a path relative to the Data root (provided
    // in argv by -D option), construct a OS independent path
    // to the file specified by "name". "name" should not start
    // with a path seperator and if path seperators are needed
    // use '/'. Be sure to delete [] the return when you are
    // finished.
    char *GetDataFilePath(const char *name)
    {
      return this->GetFilePath(this->DataRoot,name);
    }
    // Description:
    // Given a path relative to the working directory (provided
    // in argv by -T option), construct a OS independent path
    // to the file specified by "name". "name" should not start
    // with a path seperator and if path seperators are needed
    // use '/'. Be sure to delete [] the return when you are
    // finished.
    char *GetTempFilePath(const char *name)
    {
      return this->GetFilePath(this->TempRoot,name);
    }
    // Description:
    // For each point data array, take the difference of each element 
    // and sum the result. r = Sum_i(a_i-b_i). Returns true if each 
    // result is less than tol.
    bool ComparePointData(vtkPolyData *pdA, vtkPolyData *pdB, double tol);
    // Description:
    // Take the difference of each component of each tuple and 
    // sum the result. r_j = Sum_i(a_ij-b_ij). Returns true if each 
    // result is less than tol.
    bool CompareDataArrays(vtkDataArray *daA, vtkDataArray *daB, double tol);

  private:
    vtkIdType SummedDifference(vtkDataArray *daA,vtkDataArray *daB,vtkstd::vector<double> &r);
    char GetPathSep();
    char *GetDataRoot();
    char *GetTempRoot();
    char *GetCommandTailArgument(const char *tag);
    char *GetFilePath(const char *base, const char *name);
    // Internal state
    vtkPVTestUtilities();
    int Argc;
    char **Argv;
    char *DataRoot;
    char *TempRoot;
};

#endif
