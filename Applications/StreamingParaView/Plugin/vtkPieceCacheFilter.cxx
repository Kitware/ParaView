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

#include "vtkStreamingOptions.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSet.h"
#include "vtkPolyData.h"
#include "vtkAppendPolyData.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPieceCacheFilter);

#define DEBUGPRINT_CACHING(arg) \
  if (vtkStreamingOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

//----------------------------------------------------------------------------
vtkPieceCacheFilter::vtkPieceCacheFilter()
{
  this->CacheSize = -1;
  this->TryAppend = 1;
  this->AppendFilter = NULL;
  this->AppendSlot = -1;
  this->EnableStreamMessages = 0;
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_DATASET(), 1);
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
  os << indent << "TryAppend: " << (this->TryAppend?"On":"Off") << endl;
  os << indent << "AppendSlot: " << this->AppendSlot << endl;
  os << indent << "Messages: " << this->EnableStreamMessages << endl;
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
  DEBUGPRINT_CACHING(
  cerr << "PCF(" << this << ") Empty cache" << endl;
                     );

  CacheType::iterator pos;
  for (pos = this->Cache.begin(); pos != this->Cache.end(); )
    {
    pos->second.second->Delete();
    this->Cache.erase(pos++);
    }

  //remember that there is no appended data slot
  this->AppendSlot = -1;
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
void vtkPieceCacheFilter::DeletePiece(int pieceNum )
{
  DEBUGPRINT_CACHING(
  cerr << "PCF(" << this << ") Delete piece " 
  << this->ComputePiece(pieceNum) << "/" 
  << this->ComputeNumberOfPieces(pieceNum) << endl;
                     );

  CacheType::iterator pos = this->Cache.find(pieceNum);
  if (pos != this->Cache.end())
    {
    pos->second.second->Delete();
    this->Cache.erase(pos);
    }
  if (pieceNum == this->AppendSlot)
    {
    if (this->EnableStreamMessages)
      {      
      cerr << "PCF(" << this << ") Reset AppendSlot " << endl;
      }
    this->AppendSlot = -1;
    }
}

//----------------------------------------------------------------------------
int vtkPieceCacheFilter
::RequestUpdateExtent (vtkInformation *request,
                       vtkInformationVector **inputVector,
                       vtkInformationVector *outputVector)
{
  // get the info objects
  // Look through the cached data and invalidate anything too old.
  CacheType::iterator pos;
  vtkDemandDrivenPipeline *ddp = 
    vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (!ddp)
    {
    return 1;
    }

  unsigned long pmt = ddp->GetPipelineMTime();
  for (pos = this->Cache.begin(); pos != this->Cache.end();)
    {
    if (pos->second.first < pmt)
      {
      if (this->EnableStreamMessages)
        {      
        cerr << "PCF(" << this << ") Delete stale piece " << pos->first << endl;
        }
      if (pos->first == this->AppendSlot)
        {
        if (this->EnableStreamMessages)
          {      
          cerr << "PCF(" << this << ") Reset Append Slot " << pos->first << endl;
          }

        //reset the appendslot when it is cleared
        this->AppendSlot = -1;
        }

      pos->second.second->Delete();
      this->Cache.erase(pos++);
      }
    else
      {
      ++pos;
      }
    }

  //let superclass take over from here
  return 
    this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPieceCacheFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  //Fetch the data from the cache if possible and pass it on to the output.
  //Otherwise, save a copy of the input data, and pass it on to the output.
  //If TryAppend is on, append all polydata as it is added to the cache to 
  //the content of the appended slot.

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *inData = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT())
    );

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *outData = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT())
    );
  
  // fill in the request by using the cached data or input data
  int pieceNum = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  
  if (this->EnableStreamMessages)
    {
    cerr << "PCF(" << this << ") Looking for " 
         << outInfo->Get(
           vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) << "/"
         << outInfo->Get(
           vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()) << "+"
         << outInfo->Get(
           vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())
         << endl;
    }

  CacheType::iterator pos = this->Cache.find(pieceNum);
  if (pos != this->Cache.end())
    {
    DEBUGPRINT_CACHING(
    cerr << "PCF(" << this << ") Cache hit for piece " << pieceNum << endl;
                       );

    // update the m time in the cache
    pos->second.first = outData->GetUpdateTime();

    //pass the cached data onward
    DEBUGPRINT_CACHING( 
    cerr << "PCF(" << this << ") returning cached result" << endl;
                      );
    outData->ShallowCopy(pos->second.second);
    return 1;
    }
  else
    {
    if (this->EnableStreamMessages)
      {
      cerr << "PCF(" << this << ") Cache miss for piece " << pieceNum << endl;
      }
    }

  //On first update, remember that the currect piece will be used to store the 
  //appended polydata
  if (this->TryAppend && this->AppendSlot == -1)
    {
    if (this->EnableStreamMessages)
      {
      cerr << "PCF(" << this << ") NEW APPEND SLOT = " << pieceNum << endl;
      }
    this->AppendSlot = pieceNum;
    }

  //if there is space, store a copy of the data for later reuse
  if ((this->CacheSize < 0 ||
      this->Cache.size() < static_cast<unsigned long>(this->CacheSize)))
    {
    vtkDataSet *cpy = inData->NewInstance();
    cpy->ShallowCopy(inData);
    vtkInformation* dataInfo = inData->GetInformation();
    vtkInformation* cpyInfo = cpy->GetInformation();
    cpyInfo->Copy(dataInfo);
    this->Cache[pieceNum] = 
      vtkstd::pair<unsigned long, vtkDataSet *>
      (outData->GetUpdateTime(), cpy);
    if (this->EnableStreamMessages)
      {
      cerr << "PCF(" << this 
           << ") Cache insert for piece " << pieceNum << " " 
           << cpy->GetNumberOfPoints() << endl;
      }

    //if the data is polygonal, append it to a summed result
    //the summed result can then be displayed in 1 pass 
    vtkPolyData *pdIn = vtkPolyData::SafeDownCast(cpy);
    if (this->TryAppend && pdIn) 
      {
      if (this->EnableStreamMessages)
        {
        cerr << "PCF(" << this << ") MERGING New output has " << pdIn->GetNumberOfPoints() << " points" << endl;
        }

      if (!this->AppendFilter)
        {
        if (this->EnableStreamMessages)
          {
          cerr << "PCF(" << this << ") CREATE APPENDFILTER" << endl;
          }
        this->AppendFilter = vtkAppendPolyData::New();
        this->AppendFilter->UserManagedInputsOn();
        this->AppendFilter->SetNumberOfInputs(2);
        }
      //get a hold of what we have appended previously
      vtkPolyData *prevSum = vtkPolyData::SafeDownCast(
        this->GetPiece(this->AppendSlot)
        );
      if (prevSum && (this->AppendSlot != pieceNum))
        {
        vtkPolyData *newSum = NULL;
        if (this->EnableStreamMessages)
          {
          cerr << "PCF(" << this << ") SUM has " << prevSum->GetNumberOfPoints() << " points" << endl;
          }
        this->AppendFilter->SetInputByNumber(0, prevSum);
        this->AppendFilter->SetInputByNumber(1, pdIn);
        this->AppendFilter->Update();
        newSum = vtkPolyData::SafeDownCast(this->AppendFilter->GetOutput());
        if (this->EnableStreamMessages)
          {          
          cerr << "PCF(" << this << ") NewSum has " << newSum->GetNumberOfPoints() << " points" << endl;
          }

        prevSum->ShallowCopy(newSum);
        //replace old contents with new
        this->Cache[this->AppendSlot] = 
          vtkstd::pair<unsigned long, vtkDataSet *>
          (outData->GetUpdateTime(), prevSum);       

        outData->ShallowCopy(prevSum);
        return 1;

        }
      }
    else
      {
      //not appending or not polydata
      //fall through and copy input to output
      }
    }
  else
    {
    DEBUGPRINT_CACHING(
    cerr << "PCF(" << this << ") Cache full for piece " << pieceNum << endl;
                       );
    }
  
  outData->ShallowCopy(inData);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPieceCacheFilter::ProcessRequest(vtkInformation* request,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{  
  if(request->Has(vtkStreamingDemandDrivenPipeline::
     REQUEST_UPDATE_EXTENT_INFORMATION())
     &&
     this->TryAppend)
    {
    //If we already have the piece in the cache AND we have added it to 
    //the appended slot, say that the priority is 0 so that we don't 
    //waste a pass for it. The pass that processes the appended data will
    //process the data instead. Meanwhile make sure the append slot
    //has a priority of 1 to make sure it is processed.

    // compute the priority for this UE
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    if (!inInfo)
      {
      return 1;
      }

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    if (!outInfo)
      {
      return 1;
      }

    int pieceNum = 0;
    if(!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
      {
      return 1;
      }

    pieceNum = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

    vtkPolyData *pd = vtkPolyData::SafeDownCast(this->GetPiece(pieceNum));
    if (pd)
      {
      if (pieceNum == this->AppendSlot)
        {
        if (this->EnableStreamMessages)
          {
          cerr << "PCF(" << this << ") RETURNING 1 for Cache Slot at piece " 
               << pieceNum << endl;
          }
        outputVector->GetInformationObject(0)->
          Set(vtkStreamingDemandDrivenPipeline::PRIORITY(), 1.0);
        return 1;
        }
      else
        {
        if (this->EnableStreamMessages)
          {
          cerr << "PCF(" << this << ") RETURNING 0 for Cached piece " 
               << pieceNum << endl;
          }
        //we had something in the cache, AND it was a polydata, therefore it 
        //was placed in the Append slot
        outputVector->GetInformationObject(0)->
          Set(vtkStreamingDemandDrivenPipeline::PRIORITY(), 0.0);
        return 1;
        }
      }

    DEBUGPRINT_CACHING(
    cerr << "PCF(" << this 
    << ") Not cached returning input filter's answer for " 
    << pieceNum << endl;
                       );

    double inPrior = 1;
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::PRIORITY()))
      {
      inPrior = inInfo->Get(vtkStreamingDemandDrivenPipeline::
                            PRIORITY());
      }
    outputVector->GetInformationObject(0)->
      Set(vtkStreamingDemandDrivenPipeline::PRIORITY(), inPrior);
    
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector,
                                          outputVector);
}
