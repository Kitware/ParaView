// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMTimeStepIndexDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMTimeStepIndexDomain);

//---------------------------------------------------------------------------
vtkSMTimeStepIndexDomain::vtkSMTimeStepIndexDomain() = default;

//---------------------------------------------------------------------------
vtkSMTimeStepIndexDomain::~vtkSMTimeStepIndexDomain() = default;

//---------------------------------------------------------------------------
void vtkSMTimeStepIndexDomain::Update(vtkSMProperty*)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->GetRequiredProperty("Input"));
  if (pp)
  {
    this->Update(pp);
  }
}

//---------------------------------------------------------------------------
void vtkSMTimeStepIndexDomain::Update(vtkSMProxyProperty* pp)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (unsigned int i = 0; i < numProxs; i++)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
    {
      vtkPVDataInformation* info =
        sp->GetDataInformation((ip ? ip->GetUncheckedOutputPortForConnection(i) : 0));
      if (!info)
      {
        continue;
      }
      int numberOfTimeSteps = info->GetNumberOfTimeSteps();

      std::vector<vtkEntry> entries;
      entries.push_back(vtkEntry(0, numberOfTimeSteps - 1));
      this->SetEntries(entries);
      return;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMTimeStepIndexDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
