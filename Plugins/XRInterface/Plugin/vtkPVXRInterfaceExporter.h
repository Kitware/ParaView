/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
class vtkPVXRInterfaceHelperLocation;
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
    std::map<int, vtkPVXRInterfaceHelperLocation>& locations, vtkRenderer* ren);

  // export the data for each saved location
  // in a form mineview can load. Bacially
  // as imple XML format with the surface geometry
  // stored as vtp files.
  void ExportLocationsAsView(vtkPVXRInterfaceHelper* helper, vtkSMViewProxy* view,
    std::map<int, vtkPVXRInterfaceHelperLocation>& locations);

protected:
  vtkPVXRInterfaceExporter(){};
  ~vtkPVXRInterfaceExporter(){};

private:
  vtkPVXRInterfaceExporter(const vtkPVXRInterfaceExporter&) = delete;
  void operator=(const vtkPVXRInterfaceExporter&) = delete;
};

#endif
