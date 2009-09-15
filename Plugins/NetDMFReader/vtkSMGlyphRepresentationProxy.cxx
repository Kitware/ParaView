/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlyphRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGlyphRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMNetDmfViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMEnumerationDomain.h"
#include <vtksys/ios/sstream>
#include "vtkSMScatterPlotRepresentationProxy.h"

inline void vtkSMProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    proxy->UpdateProperty(pname);
    }
}

vtkStandardNewMacro(vtkSMGlyphRepresentationProxy);
vtkCxxRevisionMacro(vtkSMGlyphRepresentationProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMGlyphRepresentationProxy::vtkSMGlyphRepresentationProxy()
{

}

//-----------------------------------------------------------------------------
vtkSMGlyphRepresentationProxy::~vtkSMGlyphRepresentationProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMGlyphRepresentationProxy::AddInput(unsigned int port,
                                             vtkSMSourceProxy* input,
                                             unsigned int outputPort,
                                             const char* method)
{
  vtkSMScatterPlotRepresentationProxy* scatterPlotRep = 0;
  switch (port)
    {
    case 0:
      this->Superclass::AddInput(port, input, outputPort, method);
      break;
    case 1:
      scatterPlotRep =
        vtkSMScatterPlotRepresentationProxy::SafeDownCast(
          this->ActiveRepresentation);
      if (scatterPlotRep)
        {
        scatterPlotRep->AddInput(port, input, outputPort, method);
        }
      break;
    default:
      break;
    }

}


//-----------------------------------------------------------------------------
bool vtkSMGlyphRepresentationProxy::InputTypeIsA(const char* type)
{
  vtkPVDataInformation* data = this->GetInputProxy() ? 
    this->GetInputProxy()->GetDataInformation() : 0;
  
  return data && data->DataSetTypeIsA(type);
}

//-----------------------------------------------------------------------------
int vtkSMGlyphRepresentationProxy::GetGlyphRepresentation()
{
  vtkSMProperty* repProp = this->GetProperty("Representation");
  vtkSMEnumerationDomain* enumDomain = repProp?
    vtkSMEnumerationDomain::SafeDownCast(repProp->GetDomain("enum")): 0;
  return enumDomain ? enumDomain->GetEntryValueForText("Glyph Representation") : -1;
}

//-----------------------------------------------------------------------------
void vtkSMGlyphRepresentationProxy::SetGlyphInput(vtkSMSourceProxy* source)
{
  this->Connect(source, this->ActiveRepresentation, "GlyphInput", 0);
}

//-----------------------------------------------------------------------------
void vtkSMGlyphRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMGlyphRepresentationProxy::SetCubeAxesVisibility(int visible)
{
  vtkSMScatterPlotRepresentationProxy* scatterPlotRepresentation =
    vtkSMScatterPlotRepresentationProxy::SafeDownCast(
      this->ActiveRepresentation);
  if (scatterPlotRepresentation)
    {
    vtkSMProxySetInt(scatterPlotRepresentation, "CubeAxesVisibility",
                     visible);
    }
  else
    {
    this->Superclass::SetCubeAxesVisibility(visible);
    }
}
