// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIMultiplexerSourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIMultiplexerSourceProxy);
//----------------------------------------------------------------------------
vtkSIMultiplexerSourceProxy::vtkSIMultiplexerSourceProxy() = default;

//----------------------------------------------------------------------------
vtkSIMultiplexerSourceProxy::~vtkSIMultiplexerSourceProxy() = default;

//----------------------------------------------------------------------------
void vtkSIMultiplexerSourceProxy::Select(vtkSISourceProxy* subproxy)
{
  auto self = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  auto active_algo = vtkAlgorithm::SafeDownCast(subproxy->GetVTKObject());
  self->SetInputConnection(active_algo->GetOutputPort(0));
}

//----------------------------------------------------------------------------
void vtkSIMultiplexerSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
