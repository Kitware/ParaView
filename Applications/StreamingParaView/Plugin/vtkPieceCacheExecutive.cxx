/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceCacheExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPieceCacheExecutive.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"

#include "vtkStreamingOptions.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPieceCacheFilter.h"

vtkStandardNewMacro(vtkPieceCacheExecutive);

#define DEBUGPRINT_CACHING(arg) \
  if (vtkStreamingOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

//----------------------------------------------------------------------------
vtkPieceCacheExecutive
::vtkPieceCacheExecutive()
{
}

//----------------------------------------------------------------------------
vtkPieceCacheExecutive
::~vtkPieceCacheExecutive()
{
}

//----------------------------------------------------------------------------
int vtkPieceCacheExecutive
::NeedToExecuteData(int outputPort,
                    vtkInformationVector** inInfoVec,
                    vtkInformationVector* outInfoVec)
{
  vtkPieceCacheFilter *myPCF = 
    vtkPieceCacheFilter::SafeDownCast(this->GetAlgorithm());

  // If no port is specified, check all ports.  This behavior is
  // implemented by the superclass.
  if(outputPort < 0 || !myPCF)
    {
    return this->Superclass::NeedToExecuteData(outputPort,
                                               inInfoVec, outInfoVec);
    }

  // Does the superclass want to execute? We must skip our direct superclass
  // because it looks at update extents but does not know about the cache
  if(this->vtkDemandDrivenPipeline::NeedToExecuteData(outputPort,
                                                      inInfoVec, outInfoVec))
    {
    return 1;
    }

  // Has the algorithm asked to be executed again?
  if(this->ContinueExecuting)
    {
    return 1;
    }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* dataInfo = dataObject->GetInformation();
  int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());
  if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
    {
    int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());

    // check to see if any data in the cache fits this request
    vtkDataSet *ds = myPCF->GetPiece(updatePiece);
    if (ds)
      {
      dataInfo = ds->GetInformation();
      // Check the unstructured extent.  
      // If the piece we have doesn't match what was requested
      // we need to execute.
      int dataPiece = dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER());
      int dataNumberOfPieces = 
        dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
      int dataGhostLevel = 
        dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
      if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == 
          VTK_PIECES_EXTENT && dataPiece == updatePiece &&
          dataNumberOfPieces == updateNumberOfPieces &&
          dataGhostLevel == updateGhostLevel)
        {
        vtkDataSet *dso = vtkDataSet::SafeDownCast(dataObject);
        if (dso)
          {
          // we have a match
          // Give the cached result to the requester
          dso->ShallowCopy(ds);
          DEBUGPRINT_CACHING(
          cerr << "PCE(" << this << ") cache hit piece " 
               << updatePiece << "/"
               << updateNumberOfPieces << " "
               << dso->GetNumberOfPoints() << endl;
          );
          //pipeline request can terminate now, yeah!
          return 0;
          }     
        }
      else
        {
        DEBUGPRINT_CACHING(
          cerr << "PCE(" << this << ") miss, cached has wrong extent" << endl;
          cerr << dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) << "!="
               <<  VTK_PIECES_EXTENT << "||"
               << dataPiece << "/" << dataNumberOfPieces << "!="
               << updatePiece << "/" << updateNumberOfPieces << "||"
               << dataGhostLevel << "!=" << updateGhostLevel << endl;
                           );
          myPCF->DeletePiece(updatePiece);
        }
      }
    else
      {
      DEBUGPRINT_CACHING(
      cerr << "PCE(" << this << ") miss, nothing cached for "
           << updatePiece << "/"
           << updateNumberOfPieces << endl;
                         );
      }
    }
  else if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
    {
    //WARNING: THIS CODE HASN'T BEEN TESTED RECENTLY

    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int dataExtent[6];
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);

    vtkDataSet *ds = myPCF->GetPiece(updatePiece);
    if (ds)
      {
      dataInfo = ds->GetInformation();
      dataInfo->Get(vtkDataObject::DATA_EXTENT(), dataExtent);
      if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == 
         VTK_3D_EXTENT &&
         !(updateExtent[0] < dataExtent[0] ||
           updateExtent[1] > dataExtent[1] ||
           updateExtent[2] < dataExtent[2] ||
           updateExtent[3] > dataExtent[3] ||
           updateExtent[4] < dataExtent[4] ||
           updateExtent[5] > dataExtent[5]) &&
         (updateExtent[0] <= updateExtent[1] &&
          updateExtent[2] <= updateExtent[3] &&
          updateExtent[4] <= updateExtent[5]))
        {
        vtkDataSet *dso = vtkDataSet::SafeDownCast(dataObject);
        if (dso)
          {
          // we have a match
          // Give the cached result to the requester
          dso->ShallowCopy(ds);
          DEBUGPRINT_CACHING(
            cerr << "PCE(" << this << ") SD cache hit " << updatePiece << endl;
            );
          //pipeline request can terminate now, yeah!
          return 0;
          }
        }
      }
    }

  // We do need to execute
  DEBUGPRINT_CACHING(
  cerr << "PCE(" << this << ") cache miss " << updatePiece << endl;
                     );
  return 1;
}

//----------------------------------------------------------------------------
int vtkPieceCacheExecutive::ProcessRequest(vtkInformation* request,
                                            vtkInformationVector** inInfoVec,
                                            vtkInformationVector* outInfoVec)
{
  // Prevent RUE from being short circuited as other requests are
  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    // Get the output port from which the request was made.
    this->LastPropogateUpdateExtentShortCircuited = 1;
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
      {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      }
    
    // Make sure the information on the output port is valid.
    if(!this->VerifyOutputInformation(outputPort,inInfoVec,outInfoVec))
      {
      return 0;
      }

    // If we need to execute, propagate the update extent.
    int result = 1;

    // Do not check need to execute data! Filter uses this request to keep 
    // cache valid.
    //if(this->NeedToExecuteData(outputPort,inInfoVec,outInfoVec))
    //{
      // Invoke the request on the algorithm.
      this->LastPropogateUpdateExtentShortCircuited = 0;
      result = this->CallAlgorithm(request, vtkExecutive::RequestUpstream,
                                   inInfoVec, outInfoVec);
      
      // Propagate the update extent to all inputs.
      if(result)
        {
        result = this->ForwardUpstream(request);
        }
      result = 1;
    //}
    return result;
    }

  // Let the superclass handle other requests.
  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}

