// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVLinearExtrusionFilter
 * @brief   change a default value
 *
 * vtkPVLinearExtrusionFilter is a subclass of vtkPLinearExtrusionFilter.
 * The only difference is changing the default extrusion type to vector
 * extrusion
 */

#ifndef vtkPVLinearExtrusionFilter_h
#define vtkPVLinearExtrusionFilter_h

#include "vtkPLinearExtrusionFilter.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVLinearExtrusionFilter
  : public vtkPLinearExtrusionFilter
{
public:
  static vtkPVLinearExtrusionFilter* New();
  vtkTypeMacro(vtkPVLinearExtrusionFilter, vtkPLinearExtrusionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVLinearExtrusionFilter();
  ~vtkPVLinearExtrusionFilter() override = default;

private:
  vtkPVLinearExtrusionFilter(const vtkPVLinearExtrusionFilter&) = delete;
  void operator=(const vtkPVLinearExtrusionFilter&) = delete;
};

#endif
