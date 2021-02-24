/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataExtractWriterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMExtractsController.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMWriterProxy.h"

vtkStandardNewMacro(vtkSMDataExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMDataExtractWriterProxy::vtkSMDataExtractWriterProxy() = default;

//----------------------------------------------------------------------------
vtkSMDataExtractWriterProxy::~vtkSMDataExtractWriterProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMDataExtractWriterProxy::Write(vtkSMExtractsController* extractor)
{
  auto fname = vtkSMPropertyHelper(this, "FileName").GetAsString();
  if (!fname)
  {
    vtkErrorMacro("Missing \"FileName\"!");
    return false;
  }

  auto writer = vtkSMWriterProxy::SafeDownCast(this->GetSubProxy("Writer"));
  if (!writer)
  {
    vtkErrorMacro("Missing writer sub proxy.");
    return false;
  }

  auto convertedname = this->GenerateDataExtractsFileName(fname, extractor);
  vtkSMPropertyHelper(writer, "FileName").Set(convertedname.c_str());
  writer->UpdateVTKObjects();
  writer->UpdatePipeline(extractor->GetTime());

  // On success, add to summary.
  extractor->AddSummaryEntry(this, convertedname);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMDataExtractWriterProxy::CanExtract(vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return false;
  }

  unsigned int portNumber = 0;
  if (auto port = vtkSMOutputPort::SafeDownCast(proxy))
  {
    portNumber = port->GetPortIndex();
    proxy = port->GetSourceProxy();
  }

  bool supported = false;
  if (auto input = this->GetProperty("Input"))
  {
    vtkSMUncheckedPropertyHelper helper(input);
    helper.Set(proxy, portNumber);
    supported = (input->IsInDomains() == vtkSMDomain::IN_DOMAIN);
    helper.SetNumberOfElements(0);
  }
  return supported;
}

//----------------------------------------------------------------------------
bool vtkSMDataExtractWriterProxy::IsExtracting(vtkSMProxy* proxy)
{
  unsigned int portNumber = VTK_UNSIGNED_INT_MAX;
  if (auto port = vtkSMOutputPort::SafeDownCast(proxy))
  {
    portNumber = port->GetPortIndex();
    proxy = port->GetSourceProxy();
  }

  vtkSMPropertyHelper inputHelper(this, "Input");
  if (inputHelper.GetAsProxy() == proxy &&
    (portNumber == VTK_UNSIGNED_INT_MAX || inputHelper.GetOutputPort() == portNumber))
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMDataExtractWriterProxy::SetInput(vtkSMProxy* proxy)
{
  if (proxy == nullptr)
  {
    vtkErrorMacro("Input cannot null");
    return;
  }

  unsigned int portNumber = 0;
  if (auto port = vtkSMOutputPort::SafeDownCast(proxy))
  {
    portNumber = port->GetPortIndex();
    proxy = port->GetSourceProxy();
  }
  vtkSMPropertyHelper(this, "Input").Set(proxy, portNumber);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDataExtractWriterProxy::GetInput()
{
  vtkSMPropertyHelper helper(this, "Input");
  auto producer = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy());
  auto port = helper.GetOutputPort();
  return producer ? producer->GetOutputPort(port) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMDataExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
