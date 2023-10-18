// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkPVIncubator_h
#define vtkPVIncubator_h

#include "vtkObject.h"
#include "vtkPVIncubatorCoreModule.h" // for export macro

class VTKPVINCUBATORCORE_EXPORT vtkPVIncubator : public vtkObject
{
public:
  static vtkPVIncubator* New();
  vtkTypeMacro(vtkPVIncubator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkPVIncubator(const vtkPVIncubator&) = delete;            // Not implemented
  vtkPVIncubator& operator=(const vtkPVIncubator&) = delete; // Not implemented

protected:
  vtkPVIncubator();
  ~vtkPVIncubator() override;
};

#endif
