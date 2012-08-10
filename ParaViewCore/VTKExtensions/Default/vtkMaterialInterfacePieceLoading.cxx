/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaterialInterfacePieceLoading.h"
#include "vtkMaterialInterfaceUtilities.hxx"
#include "vector"
using std::vector;
using vtksys_ios::ostream;
using vtksys_ios::cerr;
using vtksys_ios::endl;

//
ostream &operator<<(ostream &sout, vtkMaterialInterfacePieceLoading &fp)
{
  sout << "(" << fp.GetId() << "," << fp.GetLoading() << ")";

  return sout;
}
//
ostream &operator<<(ostream &sout,
  vector<vector<vtkMaterialInterfacePieceLoading> > &pla)
{
  int nProcs=pla.size();
  for (int procId=0; procId<nProcs; ++procId)
    {
    cerr << "Fragment loading on process " << procId << ":" << endl;
    int nLocalFragments=pla[procId].size();
    for (int fragmentIdx=0; fragmentIdx<nLocalFragments; ++fragmentIdx)
      {
      sout << pla[procId][fragmentIdx] << ", ";
      }
    sout << endl;
    }
  return sout;
}
//
void PrintPieceLoadingHistogram(vector<vector<vtkIdType> > &pla)
{
  // cerr << "loading array:" <<endl;
  // cerr << pla << endl;
  int nProcs=pla.size();
  // get min and max loading
  vtkIdType minLoading=(vtkIdType)1<<((sizeof(vtkIdType)*8)-2);
  vtkIdType maxLoading=0;
  for (int procId=0; procId<nProcs; ++procId)
    {
    int nPieces=pla[procId].size();
    for (int pieceId=0; pieceId<nPieces; ++pieceId)
      {
      vtkIdType loading=pla[procId][pieceId];
      if (loading>0
          && minLoading>loading)
        {
        minLoading=loading;
        }
      if (maxLoading<loading)
        {
        maxLoading=loading;
        }
      }
    }
  // generate histogram
  const vtkIdType nBins=40;
  const vtkIdType binWidth=(maxLoading-minLoading)/nBins;
  const vtkIdType r=(maxLoading-minLoading)%nBins;
  vector<int> hist(nBins,0);
  for (int procId=0; procId<nProcs; ++procId)
    {
    int nPieces=pla[procId].size();
    for (int pieceId=0; pieceId<nPieces; ++pieceId)
      {
      vtkIdType loading=pla[procId][pieceId];
      if (loading==0)
        {
        continue;
        }
      for (int binId=0; binId<nBins; ++binId)
        {
        vtkIdType binTop=minLoading+(binId+1)*binWidth+binId*r;
        if (loading<=binTop)
          {
          ++hist[binId];
          break;
          }
        }
      }
    }
  // generate bin ids
  vector<vtkIdType> binIds(nBins);
  for (int binId=0; binId<nBins; ++binId)
    {
    binIds[binId]=static_cast<int>(minLoading+(binId+1)*binWidth);
    }
  // print
  cerr << "minLoading: " << minLoading << endl;
  cerr << "maxLoading: " << maxLoading << endl;
  cerr << "binWidth:   " << binWidth << endl;
  cerr << "nBins:      " << nBins << endl;
  PrintHistogram(hist,binIds);
}
