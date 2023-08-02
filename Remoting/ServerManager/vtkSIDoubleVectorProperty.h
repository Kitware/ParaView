// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIDoubleVectorProperty
 *
 * Vector property that manage double value to be set through a method
 * on a vtkObject.
 */

#ifndef vtkSIDoubleVectorProperty_h
#define vtkSIDoubleVectorProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIVectorProperty.h"
#include "vtkSIVectorPropertyTemplate.h" // real superclass

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSIVectorPropertyTemplate<double>
#endif
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDoubleVectorProperty : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
{
public:
  static vtkSIDoubleVectorProperty* New();
  vtkTypeMacro(vtkSIDoubleVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIDoubleVectorProperty();
  ~vtkSIDoubleVectorProperty() override;

private:
  vtkSIDoubleVectorProperty(const vtkSIDoubleVectorProperty&) = delete;
  void operator=(const vtkSIDoubleVectorProperty&) = delete;
};

#endif
