// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIIdTypeVectorProperty
 *
 * IdType ServerSide Property use to set IdType array as method parameter.
 */

#ifndef vtkSIIdTypeVectorProperty_h
#define vtkSIIdTypeVectorProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIVectorProperty.h"
#include "vtkSIVectorPropertyTemplate.h" // real superclass

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSIVectorPropertyTemplate<vtkIdType, bool>
#endif
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIIdTypeVectorProperty : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
{
public:
  static vtkSIIdTypeVectorProperty* New();
  vtkTypeMacro(vtkSIIdTypeVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIIdTypeVectorProperty();
  ~vtkSIIdTypeVectorProperty() override;

private:
  vtkSIIdTypeVectorProperty(const vtkSIIdTypeVectorProperty&) = delete;
  void operator=(const vtkSIIdTypeVectorProperty&) = delete;
};

#endif
