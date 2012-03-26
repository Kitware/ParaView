/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceCacheFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPieceCacheFilter.h"
#include "vtkObjectFactory.h"

#include "vtkAppendPolyData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPieceCacheExecutive.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAdaptiveOptions.h"

#include <vector>

#define DEBUGPRINT_CACHING(arg) ;
#define DEBUGPRINT_APPENDING(arg) ;

vtkStandardNewMacro(vtkPieceCacheFilter);

//----------------------------------------------------------------------------
vtkPieceCacheFilter::vtkPieceCacheFilter()
{
  this->CacheSize = -1;
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_DATASET(), 1);
  this->AppendFilter = vtkAppendPolyData::New();
  this->AppendFilter->UserManagedInputsOn();
  this->AppendResult = NULL;
}

//----------------------------------------------------------------------------
vtkPieceCacheFilter::~vtkPieceCacheFilter()
{
  this->EmptyCache();

  if (this->AppendFilter != NULL)
    {
    this->AppendFilter->Delete();
    this->AppendFilter = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPieceCacheFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CacheSize: " << this->CacheSize << endl;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPieceCacheFilter::CreateDefaultExecutive()
{
  return vtkPieceCacheExecutive::New();
}

//----------------------------------------------------------------------------
void vtkPieceCacheFilter::SetCacheSize(int size)
{
  this->CacheSize = size;
  if (this->Cache.size() == static_cast<unsigned long>(size))
    {
    return;
    }
  this->EmptyCache();
}

//----------------------------------------------------------------------------
void vtkPieceCacheFilter::EmptyCache()
{
  DEBUGPRINT_CACHING(cerr << "PCF(" << this << ") Empty cache" << endl;);

  CacheType::iterator pos;
  for (pos = this->Cache.begin(); pos != this->Cache.end(); )
    {
    pos->second.second->Delete();
    this->Cache.erase(pos++);
    }

  this->EmptyAppend();
}

//----------------------------------------------------------------------------
void vtkPieceCacheFilter::EmptyAppend()
{
  DEBUGPRINT_APPENDING(cerr << "PCF(" << this << ") ClearAppend" << endl;);
  //clear appended result content records
  AppendIndex::iterator pos;
  for (pos = this->AppendTable.begin(); pos != this->AppendTable.end(); )
    {
    this->AppendTable.erase(pos++);
    }
  if (this->AppendResult != NULL)
    {
    this->AppendResult->Delete();
    this->AppendResult = NULL;
    }
}

//----------------------------------------------------------------------------
vtkDataSet * vtkPieceCacheFilter::GetPiece(int pieceNum )
{
  CacheType::iterator pos = this->Cache.find(pieceNum);
  if (pos != this->Cache.end())
    {
    return pos->second.second;
    }
  return NULL;
}

//----------------------------------------------------------------------------
unsigned long vtkPieceCacheFilter::GetPieceMTime(int pieceNum)
{
  CacheType::iterator pos = this->Cache.find(pieceNum);
  if (pos != this->Cache.end())
    {
    return pos->second.first;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPieceCacheFilter::DeletePiece(int pieceNum )
{
  DEBUGPRINT_CACHING
    (
     cerr << "PCF(" << this << ") Delete piece "
     << this->ComputePiece(pieceNum) << "/"
     << this->ComputeNumberOfPieces(pieceNum);
     );

  CacheType::iterator pos = this->Cache.find(pieceNum);
  if (pos != this->Cache.end())
    {
    DEBUGPRINT_CACHING
      (
       vtkDataSet* ds = pos->second.second;
       vtkInformation* dataInfo = ds->GetInformation();
       double dataResolution = dataInfo->Get(vtkDataObject::DATA_RESOLUTION());
       cerr << "@" << dataResolution;
       );
    pos->second.second->Delete();
    this->Cache.erase(pos);

    AppendIndex::iterator apos = this->AppendTable.find(pieceNum);
    if (apos != this->AppendTable.end())
      {
      //this was also in the append slot, so we have to invalidate that
      this->EmptyAppend();
      }
    }
  DEBUGPRINT_CACHING(cerr << endl;);

}

//----------------------------------------------------------------------------
int vtkPieceCacheFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  //Fetch the data from the cache if possible and pass it on to the output.
  //Otherwise, save a copy of the input data, and pass it on to the output.

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *inData = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT())
    );

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *outData = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT())
    );

  // fill in the request by using the cached data or input data
  int updatePiece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updatePieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updateGhosts = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  double updateResolution = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());


  int index = this->ComputeIndex(updatePiece, updatePieces);
  DEBUGPRINT_CACHING
    (
     cerr << "PCF(" << this << ") Looking for "
     << updatePiece << "/"
     << updatePieces << "+"
     << updateGhosts << "@"
     << updateResolution << " in slot "
     << index << endl;
     );

  CacheType::iterator pos = this->Cache.find(index);
  bool found = false;
  if (pos != this->Cache.end())
    {
    vtkDataSet* ds = pos->second.second;
    vtkInformation* dataInfo = ds->GetInformation();
    int dataPiece = dataInfo->Get(
      vtkDataObject::DATA_PIECE_NUMBER());
    int dataPieces = dataInfo->Get(
      vtkDataObject::DATA_NUMBER_OF_PIECES());
    int dataGhosts = dataInfo->Get(
      vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
    double dataResolution = dataInfo->Get(vtkDataObject::DATA_RESOLUTION());

    if (dataPiece == updatePiece &&
        dataPieces == updatePieces &&
        dataGhosts == updateGhosts &&
        dataResolution >= updateResolution)
      {
      found = true;
      DEBUGPRINT_CACHING
        (
         cerr << "PCF(" << this << ") found match @ " << dataResolution << endl;
         );
      }
    else
      {
      DEBUGPRINT_CACHING
        (
         cerr << "PCF(" << this << ") but found "
         << dataPiece << "/"
         << dataPieces << "+"
         << dataGhosts << "@"
         << dataResolution << endl;
         );
      }
    }
  else
    {
    DEBUGPRINT_CACHING
      (
       cerr << "PCF(" << this << ") Cache miss for piece "
       << updatePiece << "/" << updatePieces << "@" << updateResolution << endl;
       );
    }

  if (found)
    {
    DEBUGPRINT_CACHING
      (
       cerr << "PCF(" << this << ") Cache hit for piece "
       << updatePiece << "/" << updatePieces << "@" << updateResolution << endl;
       );

    // update the m time in the cache
    pos->second.first = outData->GetUpdateTime();

    //pass the cached data onward
    DEBUGPRINT_CACHING
      (
       cerr << "PCF(" << this << ") returning cached result" << endl;
       );

    outData->ShallowCopy(pos->second.second);
    return 1;
    }

  //if there is space, store a copy of the data for later reuse
  if ((this->CacheSize < 0 ||
      this->Cache.size() < static_cast<unsigned long>(this->CacheSize)))
    {
    DEBUGPRINT_CACHING
      (
       cerr << "PCF(" << this
       << ") Cache insert of piece "
       << updatePiece << "/" << updatePieces << "@" << updateResolution
       << " in slot " << index << endl;
       );

    vtkDataSet *cpy = inData->NewInstance();
    cpy->ShallowCopy(inData);
    vtkInformation* dataInfo = inData->GetInformation();
    vtkInformation* cpyInfo = cpy->GetInformation();
    cpyInfo->Copy(dataInfo);

    //not getting resolution at all so all searches are failing
    //double dataResolution = dataInfo->Get(vtkDataObject::DATA_RESOLUTION());
    cpyInfo->Set(vtkDataObject::DATA_RESOLUTION(), updateResolution);

    //if we are replacing something, free it.
    if (pos != this->Cache.end())
      {
      pos->second.second->Delete();
      this->Cache.erase(pos);
      }

    this->Cache[index] =
      std::pair<unsigned long, vtkDataSet *>
      (outData->GetUpdateTime(), cpy);
    }
  else
    {
    DEBUGPRINT_CACHING
      (
       cerr << "PCF(" << this << ") Cache full. Piece "
       << updatePiece << "/" << updatePieces << "@" << updateResolution
       << " could not be saved" << endl;
       );
    }

  outData->ShallowCopy(inData);
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkPieceCacheFilter::InCache(int p, int np, double r)
{
  int index = this->ComputeIndex(p, np);
  vtkDataSet *ds = this->GetPiece(index);
  if (ds)
    {
    vtkInformation* dataInfo = ds->GetInformation();
    double dataResolution = dataInfo->Get(vtkDataObject::DATA_RESOLUTION());
    DEBUGPRINT_CACHING
      (
       vtkPolyData *content = vtkPolyData::SafeDownCast(ds);
       cerr << "PCF(" << this << ") InCache(" << p << "/" << np << "@" << r
       << "->" << dataResolution << ") " << (dataResolution>=r?"T":"F")
       << " NPTS=" << (content?content->GetNumberOfPoints():-1)<< endl;
       );
    if (dataResolution >= r)
      {
      return true;
      }
    }
  DEBUGPRINT_CACHING
    (
     cerr << "PCF(" << this << ") InCache(" << p << "/" << np << "@"
     << r << ") " << "F" << endl;
     );

  return false;
}

//-----------------------------------------------------------------------------
bool vtkPieceCacheFilter::InAppend(int p, int np, double r)
{
  int index = this->ComputeIndex(p,np);
  double dataResolution = -1.0;
  AppendIndex::iterator pos = this->AppendTable.find(index);
  if (pos != this->AppendTable.end())
    {
    dataResolution = pos->second;
    }

  DEBUGPRINT_APPENDING
    (
     cerr << "PCF(" << this << ") InAppend(" << p << "/" << np << "@"
     << r << "->" << dataResolution << ") " << (dataResolution>=r?"T":"F")
     << endl;
     );

  return dataResolution >= r;
}

//-----------------------------------------------------------------------------
vtkPolyData *vtkPieceCacheFilter::GetAppendedData()
{
  DEBUGPRINT_APPENDING
    (
     cerr << "PCF(" << this << ") GetAppendedData " << this->AppendResult
     << endl;
     );
  return this->AppendResult;
}

//-----------------------------------------------------------------------------
void vtkPieceCacheFilter::AppendPieces()
{
  DEBUGPRINT_APPENDING
    (
     cerr << "PCF(" << this << ") Append " << this->Cache.size() << " Pieces"
     << endl;
    );

  CacheType::iterator pos;

  //reuse old append result if it is not stale
  bool allCached = true;
#if 1
  if (this->AppendTable.size() != this->Cache.size())
    {
    allCached = false;
    }
#else
  for (pos = this->Cache.begin(); pos != this->Cache.end(); )
    {
    vtkPolyData *content = vtkPolyData::SafeDownCast(pos->second.second);
    if (!content)
      {
      allCached = false;
      break;
      }
    vtkInformation* dataInfo = content->GetInformation();
    int dataPiece = dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER());
    int dataPieces = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
    double dataResolution = dataInfo->Get(vtkDataObject::DATA_RESOLUTION());
    if (!this->InAppend(dataPiece, dataPieces, dataResolution))
      {
      allCached = false;
      break;
      }
    pos++;
    }
#endif
  if (allCached)
    {
    return;
    }

  this->EmptyAppend();
  if (!this->Cache.size())
    {
    return;
    }

  vtkIdType cnt = 0;
  this->AppendFilter->SetNumberOfInputs(this->Cache.size());
  for (pos = this->Cache.begin(); pos != this->Cache.end(); )
    {
    vtkPolyData *content = vtkPolyData::SafeDownCast(pos->second.second);
    if (content)
      {
      this->AppendFilter->SetInputDataByNumber(cnt++, content);

      //remember that this piece is in the appended result
      vtkInformation* dataInfo = content->GetInformation();
      int dataPiece = dataInfo->Get(
        vtkDataObject::DATA_PIECE_NUMBER());
      int dataPieces = dataInfo->Get(
        vtkDataObject::DATA_NUMBER_OF_PIECES());
      double dataResolution = dataInfo->Get(
        vtkDataObject::DATA_RESOLUTION());
      int index = this->ComputeIndex(dataPiece, dataPieces);

      this->AppendTable[index] = dataResolution;

      DEBUGPRINT_APPENDING
        (
         cerr << "Appending "<< cnt << " " << dataPiece << "/" << dataPieces
         << "@" << dataResolution << " " << content->GetNumberOfPoints()
         << endl;
         );
      }
    pos++;
    }

  if (cnt)
    {
    this->AppendFilter->SetNumberOfInputs(cnt);
    this->AppendFilter->Update();
    this->AppendResult = vtkPolyData::New();
    this->AppendResult->ShallowCopy(this->AppendFilter->GetOutput());

    DEBUGPRINT_APPENDING
      (
       cerr << "PCF("<<this<<") Appended "
       << this->AppendResult->GetNumberOfPoints()
       << " verts" << endl;
       );
    }
}
