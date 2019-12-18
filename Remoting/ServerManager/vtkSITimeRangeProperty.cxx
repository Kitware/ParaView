/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeRangeProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSITimeRangeProperty.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkSITimeRangeProperty);
//----------------------------------------------------------------------------
vtkSITimeRangeProperty::vtkSITimeRangeProperty()
{
}

//----------------------------------------------------------------------------
vtkSITimeRangeProperty::~vtkSITimeRangeProperty()
{
}

//----------------------------------------------------------------------------
void vtkSITimeRangeProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSITimeRangeProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
  {
    return false;
  }

  // Get reference to the algorithm
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(this->GetVTKObject());

  if (!algo)
  {
    return false;
  }

  vtkInformation* outInfo = algo->GetExecutive()->GetOutputInformation(0);

  // If no information just exit
  if (!outInfo)
  {
    return false;
  }

  // Else find out
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    const double* timeSteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double timeRange[2];
    if (len > 0)
    {
      timeRange[0] = timeSteps[0];
      timeRange[1] = timeSteps[len - 1];
    }
    else
    {
      timeRange[0] = timeRange[1] = 0;
    }
    // Create property and add it to the message
    ProxyState_Property* prop = msgToFill->AddExtension(ProxyState::property);
    prop->set_name(this->GetXMLName());
    Variant* var = prop->mutable_value();
    var->set_type(Variant::FLOAT64);
    var->add_float64(timeRange[0]);
    var->add_float64(timeRange[1]);
  }
  else if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    const double* timeRange = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    if (len != 2)
    {
      vtkWarningMacro(<< "Filter reports inappropriate time range.");
    }
    else
    {
      // Create property and add it to the message
      ProxyState_Property* prop = msgToFill->AddExtension(ProxyState::property);
      prop->set_name(this->GetXMLName());
      Variant* var = prop->mutable_value();
      var->set_type(Variant::FLOAT64);
      var->add_float64(timeRange[0]);
      var->add_float64(timeRange[1]);
    }
  }
  return true;
}
