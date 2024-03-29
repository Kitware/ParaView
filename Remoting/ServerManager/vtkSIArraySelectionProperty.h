// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIArraySelectionProperty
 * @brief   InformationOnly property
 *
 * SIProperty that deals with ArraySelection object
 * vtkSIDataArraySelectionProperty is recommended instead of
 * vtkSIArraySelectionProperty.
 */

#ifndef vtkSIArraySelectionProperty_h
#define vtkSIArraySelectionProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIArraySelectionProperty : public vtkSIProperty
{
public:
  static vtkSIArraySelectionProperty* New();
  vtkTypeMacro(vtkSIArraySelectionProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIArraySelectionProperty();
  ~vtkSIArraySelectionProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSIArraySelectionProperty(const vtkSIArraySelectionProperty&) = delete;
  void operator=(const vtkSIArraySelectionProperty&) = delete;
};

#endif
