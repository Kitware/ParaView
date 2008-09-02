
#include "vtkPVTestUtilities.h"

#include "vtkType.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkstd/string"//
using vtkstd::string;
using vtkstd::vector;


//
char *vtkPVTestUtilities::GetDataRoot()
{
  return this->GetCommandTailArgument("-D");
}
//
char *vtkPVTestUtilities::GetTempRoot()
{
  return this->GetCommandTailArgument("-T");
}
//
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
//
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
// Concat the data root path to the name supplied.
// The return is a c string that has the correct
// path seperators.
char *vtkPVTestUtilities::GetFilePath(
        const char *base,
        const char *name)
{
  int baseLen=strlen(base);
  int nameLen=strlen(name);
  int pathLen=baseLen+1+nameLen;
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
  return filePath;
}
//For each component of the arrays r_j=Sum_i(a_ji-b_ji). If the computation
// can't be made due to structural difficulties then a -1 is returned, otherwise
// the number of tuples differenced is returned.
vtkIdType vtkPVTestUtilities::SummedDifference(
        vtkDataArray *daA,
        vtkDataArray *daB,
        vector<double> &r)
{
  vtkIdType nTupsA=daA->GetNumberOfTuples();
  vtkIdType nTupsB=daB->GetNumberOfTuples();
  int nCompsA=daA->GetNumberOfComponents();
  int nCompsB=daB->GetNumberOfComponents();
  //
  if ((nTupsA!=nTupsB)
     || (nCompsA!=nCompsB))
  {
    return -1;
  }
  //
  double *tupA=new double [nCompsA];
  double *tupB=new double [nCompsA];
  //
  r.clear();
  r.resize(nCompsA,0.0);
  //
  for (vtkIdType i=0; i<nTupsA; ++i)
  {
    daA->GetTuple(i,tupA);
    daB->GetTuple(i,tupB);
    for (int q=0; q<nCompsA; ++q)
    {
      r[q]+=tupA[q]-tupB[q];
    }
  }
  //
  delete [] tupA;
  delete [] tupB;

  return nTupsA;
}

/**
Take the difference of each component of each tuple and 
sum the result. r_j = Sum_i(a_ij-b_ij). Returns true if each 
result is less than tol. Note: COuld take the average, but sum should be
very close ot zero if the error is due to rounding, because rounding errors
are "random" about zero with a deviation of +- k*machine eps. k is a function
of the operation *,+ etc
*/
bool vtkPVTestUtilities::CompareDataArrays(
        vtkDataArray *daA,
        vtkDataArray *daB,
        double tol)
{
  vector<double> r;
  //
  vtkIdType N=SummedDifference(daA,daB,r);
  if (N<=0)
  {
    return false;
  }
  //
  for (int q=0; q<3; ++q)
  {
    if (fabs(r[q])>tol)
    {
      cerr << r[q] << endl;
      return false;
    }
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
  daA=pdA->GetPoints()->GetData();
  daB=pdB->GetPoints()->GetData();
  //
  status=CompareDataArrays(daA,daB,tol);
  if (status==false)
  {
    return false;
  }

  // Point data arrays.
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

