// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMySphereSource_h
#define vtkMySphereSource_h

#include <vtkSphereSource.h>

#include "SphereSourceModule.h" // for export macro

/**
 * A custom sphere source that prints the content of a Qt resource
 * in its constructor.
 */
class SPHERESOURCE_EXPORT vtkMySphereSource : public vtkSphereSource
{
public:
  static vtkMySphereSource* New();
  vtkTypeMacro(vtkMySphereSource, vtkSphereSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMySphereSource();
  ~vtkMySphereSource();

private:
  vtkMySphereSource(const vtkMySphereSource&) = delete;
  void operator=(const vtkMySphereSource&) = delete;
};

#endif
