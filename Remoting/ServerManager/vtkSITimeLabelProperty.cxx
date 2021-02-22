/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeLabelProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSITimeLabelProperty.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformationKeys.h"
#include "vtkSMMessage.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkSITimeLabelProperty);
//----------------------------------------------------------------------------
vtkSITimeLabelProperty::vtkSITimeLabelProperty() = default;

//----------------------------------------------------------------------------
vtkSITimeLabelProperty::~vtkSITimeLabelProperty() = default;

//----------------------------------------------------------------------------
void vtkSITimeLabelProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSITimeLabelProperty::Pull(vtkSMMessage* msgToFill)
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
  var->set_type(Variant::STRING);

  // Else find out
  if (outInfo->Has(vtkPVInformationKeys::TIME_LABEL_ANNOTATION()))
  {
    const char* label = outInfo->Get(vtkPVInformationKeys::TIME_LABEL_ANNOTATION());
    var->add_txt(label);
    return true;
  }

  // No value does not mean failure. So just return true
  return true;
}
