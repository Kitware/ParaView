/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentUtils.hxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkCTHFragmentUtil_h
#define __vtkCTHFragmentUtil_h

// Vtk
#include<vtkCommunicator.h>
// Vtk containers
#include<vtkDoubleArray.h>
#include<vtkFloatArray.h>
#include<vtkIntArray.h>
#include<vtkUnsignedIntArray.h>
#include<vtkDataArraySelection.h>
// STL
#include "vtksys/ios/fstream"
using vtksys_ios::ofstream;
using vtksys_ios::ifstream;
#include "vtksys/ios/sstream"
using vtksys_ios::ostringstream;
#include<vtkstd/vector>
using vtkstd::vector;
#include<vtkstd/string>
using vtkstd::string;
// other
#include<assert.h>

// some useful functionality that stradles multiple filters
// and has file scope.
namespace {
// vector memory management helper
template<class T>
void ClearVectorOfVtkPointers( vector<T *> &V )
{
  int n=V.size();
  for (int i=0; i<n; ++i)
    {
    if (V[i]!=0)
      {
      V[i]->Delete();
      }
    }
  V.clear();
}
// vector memory management helper
template<class T>
void ClearVectorOfArrayPointers( vector<T *> &V )
{
  int n=V.size();
  for (int i=0; i<n; ++i)
    {
    if (V[i]!=0)
      {
      delete [] V[i];
      }
    }
  V.clear();
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkPointers(vector<T *>&V, int n)
{
  ClearVectorOfVtkPointers(V);

  V.resize(n);
  for (int i=0; i<n; ++i)
    {
    V[i]=T::New();
    }
}
// vector memory managment helper
template<class T>
void ResizeVectorOfArrayPointers(vector<T *>&V, int nV, int nA)
{
  ClearVectorOfArrayPointers(V);

  V.resize(nV);
  for (int i=0; i<nV; ++i)
    {
    V[i] = new T[nA];
    }
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkArrayPointers(
        vector<T *>&V,
        int nComps,
        vtkIdType nTups,
        string name,
        int nv)
{
  ClearVectorOfVtkPointers(V);

  V.resize(nv);
  for (int i=0; i<nv; ++i)
    {
    V[i]=T::New();
    V[i]->SetNumberOfComponents(nComps);
    V[i]->SetNumberOfTuples(nTups);
    V[i]->SetName(name.c_str());
    }
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkArrayPointers(
        vector<T *>&V,
        int nComps,
        int nv)
{
  ResizeVectorOfVtkArrayPointers(V,nComps,0,"",nv);
}
// vector memory managment helper
template<class T>
void ResizeVectorOfVtkArrayPointers(
        vector<T *>&V,
        int nComps,
        int nTups,
        int nv)
{
  ResizeVectorOfVtkArrayPointers(V,nComps,nTups,"",nv);
}

// vtk object memory managment helper
template<class T>
inline
void ReNewVtkPointer(T *&pv)
{
  if (pv!=0)
    {
    pv->Delete();
    }
  pv=T::New();
}
// vtk object memory managment helper
template<class T>
inline
void NewVtkArrayPointer(
                T *&pv,
                int nComps,
                vtkIdType nTups,
                vtkstd::string name)
{
  pv=T::New();
  pv->SetNumberOfComponents(nComps);
  pv->SetNumberOfTuples(nTups);
  pv->SetName(name.c_str());
}
// vtk object memory managment helper
template<class T>
inline
void ReNewVtkArrayPointer(
                T *&pv,
                int nComps,
                vtkIdType nTups,
                vtkstd::string name)
{
  if (pv!=0)
    {
    pv->Delete();
    }
  NewVtkArrayPointer(pv,nComps,nTups,name);
}
// vtk object memory managment helper
template<class T>
inline
void ReNewVtkArrayPointer(T *&pv,vtkstd::string name)
{
  ReNewVtkArrayPointer(pv,1,0,name);
}
// vtk object memory managment helper
template<class T>
inline
void ReleaseVtkPointer(T *&pv)
{
  assert("Attempted to release a 0 pointer."
         && pv!=0 );
  pv->Delete();
  pv=0;
}
// vtk object memory managment helper
template<class T>
inline
void CheckAndReleaseVtkPointer(T *&pv)
{
  if (pv==0)
    {
    return;
    }
  pv->Delete();
  pv=0;
}
// memory managment helper
template<class T>
inline
void CheckAndReleaseArrayPointer(T *&pv)
{
  if (pv==0)
    {
    return;
    }
  delete [] pv;
  pv=0;
}
// zero vector
template<class T>
inline
void FillVector( vector<T> &V, const T &v)
{
  int n=V.size();
  for (int i=0; i<n; ++i)
    {
    V[i]=v;
    }
}
// Copier to copy from an array where type is not known 
// into a destination buffer.
// returns 0 if the type of the source array
// is not supported.
template<class T>
inline
int CopyTuple(T *dest,          // scalar/vector
              vtkDataArray *src,//
              int nComps,       //
              int srcCellIndex) // weight of contribution
{
  // convert cell index to array index
  int srcIndex=nComps*srcCellIndex;
  // copy
  switch ( src->GetDataType() )
    {
    case VTK_FLOAT:{
      float *thisTuple
        = dynamic_cast<vtkFloatArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    case VTK_DOUBLE:{
      double *thisTuple
        = dynamic_cast<vtkDoubleArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    case VTK_INT:{
      int *thisTuple
        = dynamic_cast<vtkIntArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    case VTK_UNSIGNED_INT:{
      unsigned int *thisTuple
        = dynamic_cast<vtkUnsignedIntArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]=static_cast<T>(thisTuple[q]);
        }}
    break;
    default:
    assert( "This data type is unsupported."
            && 0 );
    return 0;
    break;
    }
  return 1;
}
// vtk data array selection helper
int GetEnabledArrayNames(vtkDataArraySelection *das, vector<string> &names)
{
  int nEnabled=das->GetNumberOfArraysEnabled();
  names.resize(nEnabled);
  int nArraysTotal=das->GetNumberOfArrays();
  for (int i=0,j=0; i<nArraysTotal; ++i)
    {
    // skip disabled arrays
    if ( !das->GetArraySetting(i) )
      {
      continue;
      }
    // save names of enabled arrays, inc name count
    names[j]=das->GetArrayName(i);
    ++j;
    }
  return nEnabled;
}
//
int WritePidFile(vtkCommunicator *comm, string pidFileName)
{
  // build an identifier string
  // "host : rank : pid"
  int nProcs=comm->GetNumberOfProcesses();
  int myProcId=comm->GetLocalProcessId();
  const int hostNameSize=256;
  char hostname[hostNameSize]={'\0'};
  gethostname(hostname, hostNameSize);
  int pid=getpid();
  const int hrpSize=512;
  char hrp[hrpSize]={'\0'};
  sprintf(hrp,"%s : %d : %d",hostname,myProcId,pid);
  // move all identifiers to controller
  char *hrpBuffer=0;
  if (myProcId==0)
    {
    hrpBuffer=new char [nProcs*hrpSize];
    }
  comm->Gather(hrp,hrpBuffer,hrpSize,0);
  // put identifiers into a file 
  if (myProcId==0)
    {
    // open a file in the current working directory
    ofstream hrpFile;
    hrpFile.open(pidFileName.c_str());
    char *thisHrp=hrpBuffer;
    if (hrpFile.is_open())
      {
      for (int procId=0; procId<nProcs; ++procId)
        {
        hrpFile << thisHrp << endl;
        thisHrp+=hrpSize;
        };
      hrpFile.close();
      }
    // if we can't open a file send to stderr
    else
      {
      for (int procId=0; procId<nProcs; ++procId)
        {
        cerr << thisHrp << endl;
        thisHrp+=hrpSize;
        }
      }
    delete [] hrpBuffer;
    }

  return pid;
}
//
string GetMemoryUsage(int pid, int line, int procId)
{
  ostringstream memoryUsage;

  ostringstream statusFileName;
  statusFileName << "/proc/" << pid << "/status";
  ifstream statusFile;
  statusFile.open(statusFileName.str().c_str());
  if (statusFile.is_open())
    {
    const int cbufSize=1024;
    char cbuf[cbufSize]={'\0'};
    while (statusFile.good())
      {
      statusFile.getline(cbuf,cbufSize);
      string content(cbuf);
      if (content.find("VmSize")!=string::npos
          || content.find("VmRSS")!=string::npos
          || content.find("VmData")!=string::npos)
        {
        int tabStart=content.find_first_of("\t ");
        int tabSpan=1;
        while (content[tabStart+tabSpan]=='\t'
               || content[tabStart+tabSpan]==' ')
          {
          ++tabSpan;
          }
        string formattedContent
          = content.substr(0,tabStart-1)+" "+content.substr(tabStart+tabSpan);
        memoryUsage << "[" << line << "] "
                    << procId << " "
                    << formattedContent
                    << endl;
        }
      }
    statusFile.close();
    }
  else
    {
    cerr << "[" << line << "] "
         << procId
         << " could not open "
         << statusFileName
         << "." << endl;
    }

  return memoryUsage.str();
}
//
void PrintHistogram(vector<int> &bins)
{
  const int maxWidth=40;
  const int n=bins.size();
  if (n==0)
    {
    return;
    }
  int maxBin=*max_element(bins.begin(),bins.end());
  for (int i=0; i<n; ++i)
    {
    if (bins[i]==0)
      {
      continue;
      }
    // clip at width of 40.
    int wid= maxBin<maxWidth ? bins[i] : bins[i]*maxWidth/maxBin;
    cerr << "[" << i << "]*";
    for (int j=1; j<wid; ++j)
      {
      cerr << "*";
      }
    cerr << "(" << bins[i] << ")" << endl;
    }
  return;
}
};
#endif
