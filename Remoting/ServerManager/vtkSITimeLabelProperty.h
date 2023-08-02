// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSITimeLabelProperty
 *
 * SIProperty that deals with TimeLabel annotation on Algorithm object type
 */

#ifndef vtkSITimeLabelProperty_h
#define vtkSITimeLabelProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSITimeLabelProperty : public vtkSIProperty
{
public:
  static vtkSITimeLabelProperty* New();
  vtkTypeMacro(vtkSITimeLabelProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSITimeLabelProperty();
  ~vtkSITimeLabelProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSITimeLabelProperty(const vtkSITimeLabelProperty&) = delete;
  void operator=(const vtkSITimeLabelProperty&) = delete;
};

#endif
