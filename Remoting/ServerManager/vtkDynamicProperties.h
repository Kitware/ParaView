// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDynamicProperties
 * @brief   Key names and types for dynamic properties
 *
 * See:
 * - vtkPVRenderView::GetANARIRendererParameters for the JSON
 * description of properties associated with an ANARI renderer
 * - pqDynamicPropertiesWidget for the panel_widget for these
 * properties and
 * - ANARIRenderParameter XML property definiton for the RenderViewProxy
 * in view_removingviews.xml
 *
 */

#ifndef vtkDynamicProperties_h
#define vtkDynamicProperties_h

#include "vtkRemotingServerManagerModule.h" // For export macro

#define VTK_DYNAMIC_PROPERTIES_MAJOR_VERSION 1
#define VTK_DYNAMIC_PROPERTIES_MINOR_VERSION 0
#define VTK_DYNAMIC_PROPERTIES_VERSION_CHECK(major, minor) (100000000 * major + minor)
#define VTK_DYNAMIC_PROPERTIES_VERSION_NUMBER_QUICK                                                \
  (VTK_DYNAMIC_PROPERTIES_VERSION_CHECK(                                                           \
    VTK_DYNAMIC_PROPERTIES_MAJOR_VERSION, VTK_DYNAMIC_PROPERTIES_MINOR_VERSION))

struct VTKREMOTINGSERVERMANAGER_EXPORT vtkDynamicProperties
{
  static constexpr const char* VERSION_KEY = "version";
  static constexpr const char* PROPERTIES_KEY = "properties";
  static constexpr const char* NAME_KEY = "name";
  static constexpr const char* TYPE_KEY = "type";
  static constexpr const char* DESCRIPTION_KEY = "description";
  static constexpr const char* DEFAULT_KEY = "default";
  static constexpr const char* VALUE_KEY = "value";
  static constexpr const char* MIN_KEY = "min";
  static constexpr const char* MAX_KEY = "max";

  enum Type
  {
    BOOL,
    INT,
    DOUBLE,
    INVALID_TYPE,
  };
};

#endif // vtkDynamicProperties_h
