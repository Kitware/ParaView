// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPSWriterProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMPSWriterProxy);
//-----------------------------------------------------------------------------
vtkSMPSWriterProxy::vtkSMPSWriterProxy() = default;

//-----------------------------------------------------------------------------
vtkSMPSWriterProxy::~vtkSMPSWriterProxy() = default;

//-----------------------------------------------------------------------------
void vtkSMPSWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
