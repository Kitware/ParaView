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
void vtkSMSpreadSheetRepresentationProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  if (name && strcmp(name, "Input") == 0)
  {
    vtkSMPropertyHelper helper(this, name);
    for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
    {
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(cc));
      if (input)
      {
        input->CreateSelectionProxies();
        vtkSMSourceProxy* esProxy = input->GetSelectionOutput(helper.GetOutputPort(cc));
        if (!esProxy)
        {
          vtkErrorMacro("Input proxy does not support selection extraction.");
        }
        else
        {
          // We use these internal properties since we need to add consumer dependency
          // on this proxy so that MarkModified() is called correctly.
          vtkSMPropertyHelper(this, "InternalInput1").Set(esProxy, 0);
          this->UpdateProperty("InternalInput1");
        }
      }
    }
  }
  this->Superclass::SetPropertyModifiedFlag(name, flag);
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
