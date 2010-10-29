/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSpreadSheetRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMSpreadSheetRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::vtkSMSpreadSheetRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::~vtkSMSpreadSheetRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::AddInput(unsigned int inputPort,
  vtkSMSourceProxy* input, unsigned int outputPort, const char* method)
{
  this->Superclass::AddInput(inputPort, input, outputPort, method);
  input->CreateSelectionProxies();

  if (inputPort == 0)
    {
    vtkSMSourceProxy* esProxy = input->GetSelectionOutput(outputPort);
    if (!esProxy)
      {
      vtkErrorMacro("Input proxy does not support selection extraction.");
      return;
      }

    // We use these internal properties since we need to add consumer dependecy
    // on this proxy so that MarkModified() is called correctly.
    vtkSMPropertyHelper(this, "InternalInput1").Set(esProxy, 0);
    vtkSMPropertyHelper(this, "InternalInput2").Set(esProxy, 1);
    this->UpdateProperty("InternalInput1");
    this->UpdateProperty("InternalInput2");
    }
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


