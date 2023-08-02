// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVProxyDefinitionIterator.h"

#include "vtkObjectFactory.h"
class vtkPVXMLElement;

//-----------------------------------------------------------------------------
vtkPVProxyDefinitionIterator::vtkPVProxyDefinitionIterator() = default;
//---------------------------------------------------------------------------
vtkPVProxyDefinitionIterator::~vtkPVProxyDefinitionIterator() = default;
//---------------------------------------------------------------------------
void vtkPVProxyDefinitionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
