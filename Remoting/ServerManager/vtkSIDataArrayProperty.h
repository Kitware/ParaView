// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIDataArrayProperty
 * @brief   InformationOnly property
 *
 * SIProperty that deals with vtkDataArray object type
 */

#ifndef vtkSIDataArrayProperty_h
#define vtkSIDataArrayProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDataArrayProperty : public vtkSIProperty
{
public:
  static vtkSIDataArrayProperty* New();
  vtkTypeMacro(vtkSIDataArrayProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIDataArrayProperty();
  ~vtkSIDataArrayProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSIDataArrayProperty(const vtkSIDataArrayProperty&) = delete;
  void operator=(const vtkSIDataArrayProperty&) = delete;
};

#endif
