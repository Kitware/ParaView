// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSpreadSheetRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMSpreadSheetRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::vtkSMSpreadSheetRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::~vtkSMSpreadSheetRepresentationProxy() = default;

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
