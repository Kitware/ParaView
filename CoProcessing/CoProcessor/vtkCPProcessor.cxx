/*=========================================================================

  Program:   ParaView
  Module:    vtkCPProcessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPProcessor.h"

#include "vtkCPDataDescription.h"
#include "vtkCPPipeline.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"

#include <vtkstd/set>

struct vtkCPProcessorInternals
{
  typedef vtkstd::set<vtkSmartPointer<vtkCPPipeline> > PipelineSet;
  typedef PipelineSet::iterator PipelineSetIterator;
  PipelineSet Pipelines;
};

vtkCxxRevisionMacro(vtkCPProcessor, "1.1");
vtkStandardNewMacro(vtkCPProcessor);

//----------------------------------------------------------------------------
vtkCPProcessor::vtkCPProcessor()
{
  this->Internal = new vtkCPProcessorInternals;
}

//----------------------------------------------------------------------------
vtkCPProcessor::~vtkCPProcessor()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::AddPipeline(vtkCPPipeline* Pipeline)
{
  if(!Pipeline)
    {
    vtkErrorMacro("Pipeline is NULL.");
    return 0;
    }
  this->Internal->Pipelines.insert(Pipeline);
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Initialize()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::RequestDataDescription(
  vtkCPDataDescription* DataDescription)
{
  if(!DataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }
  int DoCoProcessing = 0;
  DataDescription->Reset();
  for(vtkCPProcessorInternals::PipelineSetIterator iter = 
        this->Internal->Pipelines.begin();
      iter!=this->Internal->Pipelines.end();iter++)
    {
    if(iter->GetPointer()->RequestDataDescription(DataDescription))
      {
      DoCoProcessing = 1;
      }
    }
  return DoCoProcessing;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::CoProcess(vtkCPDataDescription* DataDescription)
{
  if(!DataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }
  int Success = 1;
  DataDescription->Reset();
  for(vtkCPProcessorInternals::PipelineSetIterator iter = 
        this->Internal->Pipelines.begin();
      iter!=this->Internal->Pipelines.end();iter++)
    {
    if(!iter->GetPointer()->CoProcess(DataDescription))
      {
      Success = 0;
      }
    }
  return Success;
}

//----------------------------------------------------------------------------
int vtkCPProcessor::Finalize()
{
  this->Internal->Pipelines.clear();
  return 1;
}

//----------------------------------------------------------------------------
void vtkCPProcessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
