// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSITimeRangeProperty
 *
 * SIProperty that deals with TimeRange on Algorithm object type
 */

#ifndef vtkSITimeRangeProperty_h
#define vtkSITimeRangeProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSITimeRangeProperty : public vtkSIProperty
{
public:
  static vtkSITimeRangeProperty* New();
  vtkTypeMacro(vtkSITimeRangeProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSITimeRangeProperty();
  ~vtkSITimeRangeProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSITimeRangeProperty(const vtkSITimeRangeProperty&) = delete;
  void operator=(const vtkSITimeRangeProperty&) = delete;
};

#endif
