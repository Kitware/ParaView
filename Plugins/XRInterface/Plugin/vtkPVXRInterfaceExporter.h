// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVXRInterfaceExporter
 * @brief   support for exporting XRInterfaceViews
 *
 */

#ifndef vtkPVXRInterfaceExporter_h
#define vtkPVXRInterfaceExporter_h

#include "vtkObject.h"
#include <map> // for ivar

class vtkSMViewProxy;
struct vtkPVXRInterfaceHelperLocation;
class vtkPVXRInterfaceHelper;
class vtkRenderer;

class vtkPVXRInterfaceExporter : public vtkObject
{
public:
  static vtkPVXRInterfaceExporter* New();
  vtkTypeMacro(vtkPVXRInterfaceExporter, vtkObject);

  // export the data for each saved location
  // as a skybox
  void ExportLocationsAsSkyboxes(vtkPVXRInterfaceHelper* helper, vtkSMViewProxy* view,
    std::vector<vtkPVXRInterfaceHelperLocation>& locations, vtkRenderer* ren);

  // export the data for each saved location
  // in a form mineview can load. Bacially
  // as imple XML format with the surface geometry
  // stored as vtp files.
  void ExportLocationsAsView(vtkPVXRInterfaceHelper* helper, vtkSMViewProxy* view,
    std::vector<vtkPVXRInterfaceHelperLocation>& locations);

protected:
  vtkPVXRInterfaceExporter() = default;
  ~vtkPVXRInterfaceExporter() override = default;

private:
  vtkPVXRInterfaceExporter(const vtkPVXRInterfaceExporter&) = delete;
  void operator=(const vtkPVXRInterfaceExporter&) = delete;
};

#endif
