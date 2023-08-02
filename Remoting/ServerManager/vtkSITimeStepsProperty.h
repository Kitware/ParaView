// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSITimeRangeProperty
 *
 * SIProperty that deals with TimeRange on Algorithm object type
 */

#ifndef vtkSITimeStepsProperty_h
#define vtkSITimeStepsProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSITimeStepsProperty : public vtkSIProperty
{
public:
  static vtkSITimeStepsProperty* New();
  vtkTypeMacro(vtkSITimeStepsProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSITimeStepsProperty();
  ~vtkSITimeStepsProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSITimeStepsProperty(const vtkSITimeStepsProperty&) = delete;
  void operator=(const vtkSITimeStepsProperty&) = delete;
};

#endif
