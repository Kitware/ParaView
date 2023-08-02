// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNetworkAccessManager.h"

//----------------------------------------------------------------------------
vtkNetworkAccessManager::vtkNetworkAccessManager() = default;

//----------------------------------------------------------------------------
vtkNetworkAccessManager::~vtkNetworkAccessManager() = default;

//----------------------------------------------------------------------------
void vtkNetworkAccessManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
