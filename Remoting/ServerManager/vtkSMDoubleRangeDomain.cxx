// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Instantiate superclass first to give the template a DLL interface.
#include "vtkSMRangeDomainTemplate.txx"
VTK_SM_RANGE_DOMAIN_TEMPLATE_INSTANTIATE(double);

#define vtkSMDoubleRangeDomain_cxx
#include "vtkSMDoubleRangeDomain.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMDoubleRangeDomain);
//---------------------------------------------------------------------------
vtkSMDoubleRangeDomain::vtkSMDoubleRangeDomain() = default;

//---------------------------------------------------------------------------
vtkSMDoubleRangeDomain::~vtkSMDoubleRangeDomain() = default;

//---------------------------------------------------------------------------
void vtkSMDoubleRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
