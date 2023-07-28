// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIIntVectorProperty
 *
 * ServerSide Property use to set int array as method argument.
 */

#ifndef vtkSIIntVectorProperty_h
#define vtkSIIntVectorProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIVectorProperty.h"
#include "vtkSIVectorPropertyTemplate.h" // real superclass

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSIVectorPropertyTemplate<int>
#endif
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIIntVectorProperty : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
{
public:
  static vtkSIIntVectorProperty* New();
  vtkTypeMacro(vtkSIIntVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIIntVectorProperty();
  ~vtkSIIntVectorProperty() override;

private:
  vtkSIIntVectorProperty(const vtkSIIntVectorProperty&) = delete;
  void operator=(const vtkSIIntVectorProperty&) = delete;
};

#endif
