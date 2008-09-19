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
//
//  Given two data sets which are assumed to be the same, point wize,
//  measure the L2 norm of the point wise difference and compare to a 
//  specified tolerance. Prior to the comparision the norm is scaled 
//  by magnitude of one of the values, which normalizes the difference
//  o the order of machine epsilon.
//

#ifndef vtkPVTestUtilities_h
#define vtkPVTestUtilities_h

#include "vtkObject.h"

class vtkPolyData;
class vtkDataArray;

class vtkPVTestUtilities : public vtkObject
{
public:
  // the usual vtk stuff
  vtkTypeRevisionMacro(vtkPVTestUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent){};
  static vtkPVTestUtilities *New();

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

  // Description:
  // Initialize the object from command tail arguments.
  void Initialize(int argc, char **argv)
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

protected:
  vtkPVTestUtilities(){ this->Initialize(0,0); }
  ~vtkPVTestUtilities(){ this->Initialize(0,0); }

private:
  vtkPVTestUtilities(const vtkPVTestUtilities &); // Not implemented
  vtkPVTestUtilities &operator=(const vtkPVTestUtilities &); // Not implemented

  // Description:
  // Sum the L2 Norm point wise over all tuples. Each term
  // is scaled by the magnitude of one of the inputs.
  // Return sum and the number of terms.
  //
  // Sum_i(Sqrt(Sum_j( b_ij - a_ij )**2)/Sqrt(Sum_j(a_ij**2)))
  template <class T, class T_vtk>
  vtkIdType AccumulateScaledL2Norm(
          T_vtk *daA, // first vtk array
          T *pA,      // pointer to its data
          T_vtk *daB, // second vtk array
          T *pB,      // pointer to its data
          double &SumModR); // result
  ///
  char GetPathSep();
  char *GetDataRoot();
  char *GetTempRoot();
  char *GetCommandTailArgument(const char *tag);
  char *GetFilePath(const char *base, const char *name);
  //
  int Argc;
  char **Argv;
  char *DataRoot;
  char *TempRoot;
};

#endif
