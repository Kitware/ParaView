/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeStepsProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSITimeStepsProperty.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkSITimeStepsProperty);
//----------------------------------------------------------------------------
vtkSITimeStepsProperty::vtkSITimeStepsProperty()
{
}

//----------------------------------------------------------------------------
vtkSITimeStepsProperty::~vtkSITimeStepsProperty()
{
}

//----------------------------------------------------------------------------
void vtkSITimeStepsProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSITimeStepsProperty::Pull(vtkSMMessage* msgToFill)
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

  // Create property and add it to the message
  ProxyState_Property* prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* var = prop->mutable_value();
  var->set_type(Variant::FLOAT64);

  // Else find out
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    const double* timeSteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    for (int i = 0; i < len; i++)
    {
      var->add_float64(timeSteps[i]);
    }
    return true;
  }

  // No value does not mean failure. So just return true
  return true;
}
