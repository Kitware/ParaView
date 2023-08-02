// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMProxyInitializationHelper.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkSMProxyInitializationHelper::vtkSMProxyInitializationHelper() = default;

//----------------------------------------------------------------------------
vtkSMProxyInitializationHelper::~vtkSMProxyInitializationHelper() = default;

//----------------------------------------------------------------------------
void vtkSMProxyInitializationHelper::RegisterProxy(vtkSMProxy*, vtkPVXMLElement*) {}

//----------------------------------------------------------------------------
void vtkSMProxyInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
