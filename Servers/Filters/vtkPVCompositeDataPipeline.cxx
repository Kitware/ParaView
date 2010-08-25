/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVCompositeDataPipeline.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeDataPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataIterator.h"
#include "vtkImageData.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTemporalDataSet.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro(vtkPVCompositeDataPipeline);


//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::vtkPVCompositeDataPipeline()
{

}

//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::~vtkPVCompositeDataPipeline()
{

}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::CopyDefaultInformation(
  vtkInformation* request, int direction,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::CopyDefaultInformation(request, direction,
                                           inInfoVec, outInfoVec);

  if (request->Has(REQUEST_INFORMATION()))
    {
    if (this->GetNumberOfInputPorts() > 0)
      {
      if (vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0))
        {
        // Copy information from the first input to all outputs.
        for(int i=0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
          {
          vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
          outInfo->CopyEntry(inInfo, COMPOSITE_DATA_META_DATA());
          }
        }
      }
    }

  if(request->Has(REQUEST_UPDATE_EXTENT()))
    {
    // Find the port that has a data that we will iterator over.
    // If there is one, make sure that we use piece extent for
    // that port. Composite data pipeline works with piece extents
    // only.
    int compositePort;
    if (this->ShouldIterateOverInput(compositePort))
      {
      // Get the output port from which to copy the extent.
      int outputPort = -1;
      if(request->Has(FROM_OUTPUT_PORT()))
        {
        outputPort = request->Get(FROM_OUTPUT_PORT());
        }

      // Setup default information for the inputs.
      if(outInfoVec->GetNumberOfInformationObjects() > 0)
        {
        // Copy information from the output port that made the request.
        // Since VerifyOutputInformation has already been called we know
        // there is output information with a data object.
        vtkInformation* outInfo =
          outInfoVec->GetInformationObject((outputPort >= 0)? outputPort : 0);

        // Loop over all connections on this input port.
        int numInConnections =
          inInfoVec[compositePort]->GetNumberOfInformationObjects();
        for (int j=0; j<numInConnections; j++)
          {
          // Get the pipeline information for this input connection.
          vtkInformation* inInfo =
            inInfoVec[compositePort]->GetInformationObject(j);

          inInfo->CopyEntry(outInfo, UPDATE_TIME_STEPS());
          inInfo->CopyEntry(outInfo, FAST_PATH_OBJECT_ID());
          inInfo->CopyEntry(outInfo, FAST_PATH_ID_TYPE());
          inInfo->CopyEntry(outInfo, FAST_PATH_OBJECT_TYPE());
          vtkDebugMacro(<< "CopyEntry UPDATE_PIECE_NUMBER() " << outInfo->Get(UPDATE_PIECE_NUMBER()) << " " << outInfo);

          inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
          inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
          inInfo->CopyEntry(outInfo, UPDATE_COMPOSITE_INDICES());
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
