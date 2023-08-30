// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMTimeStepsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMTimeStepsDomain);

//---------------------------------------------------------------------------
void vtkSMTimeStepsDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(this->GetRequiredProperty("Input"));
  if (!ip)
  {
    vtkErrorMacro("No input property specified");
    return;
  }

  vtkNew<vtkPVDataInformation> accumulatedInfo;
  unsigned int numProxs = ip->GetNumberOfUncheckedProxies();
  for (unsigned int i = 0; i < numProxs; i++)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(ip->GetUncheckedProxy(i));
    if (sp)
    {
      vtkPVDataInformation* info =
        sp->GetDataInformation((ip ? ip->GetUncheckedOutputPortForConnection(i) : 0));
      if (!info)
      {
        continue;
      }
      accumulatedInfo->AddInformation(info);
    }
  }

  std::vector<double> newValues;
  const std::set<double>& timesteps = accumulatedInfo->GetTimeSteps();
  std::copy(timesteps.begin(), timesteps.end(), std::back_inserter(newValues));
  if (newValues != this->Values)
  {
    this->Values = newValues;
    this->InvokeModified();
  }
}
