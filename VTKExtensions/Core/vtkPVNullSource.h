// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVNullSource
 * @brief   source for NULL data.
 *
 * This is a source for null data. Although this actually produces a
 * vtkPolyLine paraview blocks all data information from this source resulting
 * in it being treated as a null source.
 */

#ifndef vtkPVNullSource_h
#define vtkPVNullSource_h

#include "vtkPVVTKExtensionsCoreModule.h" //needed for exports
#include "vtkPolyDataAlgorithm.h"

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVNullSource : public vtkPolyDataAlgorithm
{
public:
  static vtkPVNullSource* New();
  vtkTypeMacro(vtkPVNullSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVNullSource();
  ~vtkPVNullSource() override;

private:
  vtkPVNullSource(const vtkPVNullSource&) = delete;
  void operator=(const vtkPVNullSource&) = delete;
};

#endif
