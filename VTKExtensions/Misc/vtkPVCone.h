// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPVCone
 * @brief   extends vtkCone to add ParaView specific behavior.
 *
 * vtkPVCone extends vtkCone to add ParaView specific behavior. Unlike vtkCone, vtkPVCone represents
 * a one-sided cone by default.
 */

#ifndef vtkPVCone_h
#define vtkPVCone_h

#include "vtkCone.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVCone : public vtkCone
{
public:
  static vtkPVCone* New();

  vtkTypeMacro(vtkPVCone, vtkCone);

private:
  vtkPVCone();
  ~vtkPVCone() override = default;

  vtkPVCone(const vtkPVCone&) = delete;
  void operator=(const vtkPVCone&) = delete;
};

#endif
