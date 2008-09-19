/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTestUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTestUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkType.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkstd/string"//
#include "vtkstd/vector"//
#include "vtkType.h"//
using vtkstd::string;
using vtkstd::vector;

vtkCxxRevisionMacro(vtkPVTestUtilities, "1.4");
vtkStandardNewMacro(vtkPVTestUtilities);


//-----------------------------------------------------------------------------
char *vtkPVTestUtilities::GetDataRoot()
{
  return this->GetCommandTailArgument("-D");
}

//-----------------------------------------------------------------------------
char *vtkPVTestUtilities::GetTempRoot()
{
  return this->GetCommandTailArgument("-T");
}

//-----------------------------------------------------------------------------
char *vtkPVTestUtilities::GetCommandTailArgument(const char *tag)
{
  for (int i=1; i<this->Argc; ++i)
  {
    if (string(this->Argv[i])==string(tag))
    {
      return this->Argv[i+1];
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
char vtkPVTestUtilities::GetPathSep()
{
  #if defined _WIN32 && !defined __CYGWIN__
  return '\\';
  #elif defined _WIN64 && !defined __CYGWIN__
  return '\\';
  #else
  return '/';
  #endif
}

//-----------------------------------------------------------------------------
// Concat the data root path to the name supplied.
// The return is a c string that has the correct
// path seperators.
char *vtkPVTestUtilities::GetFilePath(
        const char *base,
        const char *name)
{
  int baseLen=strlen(base);
  int nameLen=strlen(name);
  int pathLen=baseLen+1+nameLen+1;
  char *filePath=new char[pathLen];
  int i=0;
  for (; i<baseLen; ++i)
  {
    if ( this->GetPathSep()=='\\'
          && base[i]=='/')
    {
      filePath[i]='\\';
    }
    else
    {
      filePath[i]=base[i];
    }
  }
  filePath[i]=this->GetPathSep();
  ++i;
  for (int j=0; j<nameLen; ++j, ++i)
  {
    if ( this->GetPathSep()=='\\'
          && name[j]=='/')
    {
      filePath[i]='\\';
    }
    else
    {
      filePath[i]=name[j];
    }
  }
  filePath[i]='\0';
  return filePath;
}

//-----------------------------------------------------------------------------
// Sum the L2 Norm point wise over all tuples.
// Return the number of terms in the sum.
template <class T, class T_vtk>
vtkIdType vtkPVTestUtilities::AccumulateScaledL2Norm(
        T_vtk *daA, // first vtk array
        T *pA,      // pointer to its data
        T_vtk *daB, // second vtk array
        T *pB,      // pointer to its data
        double &SumModR) // result
{
  vtkIdType nTupsA=daA->GetNumberOfTuples();
  vtkIdType nTupsB=daB->GetNumberOfTuples();
  int nCompsA=daA->GetNumberOfComponents();
  int nCompsB=daB->GetNumberOfComponents();
  //
  if ((nTupsA!=nTupsB)
     || (nCompsA!=nCompsB))
    {
    vtkGenericWarningMacro(
              "Arrays: " << daA->GetName()
              << " (nC=" << nCompsA 
              << " nT= "<< nTupsA << ")"
              << " and " << daB->GetName()
              << " (nC=" << nCompsB 
              << " nT= "<< nTupsB << ")"
              << " do not have the same structure.");
    return -1;
    }
  //
  SumModR=0.0;
  const T *tupA=daA->GetPointer(0);
  const T *tupB=daB->GetPointer(0);
  for (vtkIdType i=0; i<nTupsA; ++i)
    {
    double modR=0.0;
    double modA=0.0;
    for (int q=0; q<nCompsA; ++q)
      {
      double a=tupA[q];
      double b=tupB[q];
      modA+=a*a;
      double r=b-a;
      modR+=r*r;
      }
    modA=sqrt(modA);
    modA= modA<1.0 ? 1.0 : modA;
    SumModR+=sqrt(modR)/modA;
    tupA+=nCompsA;
    tupB+=nCompsA;
    }
  return nTupsA;
}

//-----------------------------------------------------------------------------
bool vtkPVTestUtilities::CompareDataArrays(
        vtkDataArray *daA,
        vtkDataArray *daB,
        double tol)
{
  int typeA=daA->GetDataType();
  int typeB=daB->GetDataType();
  if (typeA!=typeB)
    {
    vtkWarningMacro("Incompatible data types: "
                    << typeA << ","
                    << typeB << ".");
    return false;
    }

  double L2=0.0;
  vtkIdType N=0;
  switch (typeA)
    {
    case VTK_DOUBLE:
      {
      vtkDoubleArray *A=dynamic_cast<vtkDoubleArray *>(daA);
      double *pA=A->GetPointer(0);
      vtkDoubleArray *B=dynamic_cast<vtkDoubleArray *>(daB);
      double *pB=B->GetPointer(0);
      N=this->AccumulateScaledL2Norm(A,pA,B,pB,L2);
      }
      break;
    case VTK_FLOAT:
      {
      vtkFloatArray *A=dynamic_cast<vtkFloatArray *>(daA);
      float *pA=A->GetPointer(0);
      vtkFloatArray *B=dynamic_cast<vtkFloatArray *>(daB);
      float *pB=B->GetPointer(0);
      N=this->AccumulateScaledL2Norm(A,pA,B,pB,L2);
      }
      break;
    default:
      cerr << "Skipping:" << daA->GetName() << endl;
      return true;
      break;
    }
  //
  if (N<=0)
  {
    return false;
  }
  //
  cerr << "Sum(L2)/N of "
       << daA->GetName()
       << " < " << tol 
       << "? = " << L2
       << "/" << N
       << "."  << endl;
  //
  double avgL2=L2/N;
  if (avgL2>tol)
    {
    return false;
    }

  // Test passed
  return true;
}
//
bool vtkPVTestUtilities::ComparePointData(
        vtkPolyData *pdA,
        vtkPolyData *pdB,
        double tol)
{
  vtkDataArray *daA=0;
  vtkDataArray *daB=0;
  bool status=false;

  // Points.
  cerr << "Comparing points:" << endl;
  daA=pdA->GetPoints()->GetData();
  daB=pdB->GetPoints()->GetData();
  //
  status=CompareDataArrays(daA,daB,tol);
  if (status==false)
    {
    return false;
    }

  // Point data arrays.
  cerr << "Comparing data arrays:" << endl;
  int nDaA=pdA->GetPointData()->GetNumberOfArrays();
  int nDaB=pdB->GetPointData()->GetNumberOfArrays();
  if (nDaA!=nDaB)
    {
    return false;
    }
  //
  for (int arrayId=0; arrayId<nDaA; ++arrayId)
    {
    daA=pdA->GetPointData()->GetArray(arrayId);
    daB=pdB->GetPointData()->GetArray(arrayId);
    //
    status=CompareDataArrays(daA,daB,tol);
    if (status==false)
      {
      return false;
      }
    }
  // All tests passed.
  return true;
}

