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
 * @class   vtkPVOpenVRExporter
 * @brief   support for exporting OpenVRViews
 *
 */

#ifndef vtkPVOpenVRExporter_h
#define vtkPVOpenVRExporter_h

#include "vtkObject.h"
#include <map> // for ivar

class vtkSMViewProxy;
class vtkPVOpenVRHelperLocation;
class vtkPVOpenVRHelper;

class vtkPVOpenVRExporter : public vtkObject
{
public:
  static vtkPVOpenVRExporter* New();
  vtkTypeMacro(vtkPVOpenVRExporter, vtkObject);

  // export the data for each saved location
  // as a skybox
  void ExportLocationsAsSkyboxes(vtkPVOpenVRHelper* helper, vtkSMViewProxy* view,
    std::map<int, vtkPVOpenVRHelperLocation>& locations);

  // export the data for each saved location
  // in a form mineview can load. Bacially
  // as imple XML format with the surface geometry
  // stored as vtp files.
  void ExportLocationsAsView(vtkPVOpenVRHelper* helper, vtkSMViewProxy* view,
    std::map<int, vtkPVOpenVRHelperLocation>& locations);

protected:
  vtkPVOpenVRExporter(){};
  ~vtkPVOpenVRExporter(){};

private:
  vtkPVOpenVRExporter(const vtkPVOpenVRExporter&) = delete;
  void operator=(const vtkPVOpenVRExporter&) = delete;
};

#endif
