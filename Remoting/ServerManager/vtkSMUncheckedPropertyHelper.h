// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMUncheckedPropertyHelper_h
#define vtkSMUncheckedPropertyHelper_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMPropertyHelper.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMUncheckedPropertyHelper : public vtkSMPropertyHelper
{
public:
  vtkSMUncheckedPropertyHelper(vtkSMProxy* proxy, const char* name, bool quiet = false);
  vtkSMUncheckedPropertyHelper(vtkSMProperty* property, bool quiet = false);

private:
  vtkSMUncheckedPropertyHelper(const vtkSMUncheckedPropertyHelper&) = delete;
  void operator=(const vtkSMUncheckedPropertyHelper&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkSMUncheckedPropertyHelper.h
