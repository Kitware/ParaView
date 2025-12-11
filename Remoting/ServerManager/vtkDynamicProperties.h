// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDynamicProperties
 * @brief   Key names and types for dynamic properties
 *
 * See:
 * - vtkPVRenderView::GetANARIRendererParameters for the json
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

struct VTKREMOTINGSERVERMANAGER_EXPORT vtkDynamicProperties
{
  static const char* LIBRARY_KEY;
  static const char* RENDERER_KEY;
  static const char* PROPERTIES_KEY;
  static const char* NAME_KEY;
  static const char* TYPE_KEY;
  static const char* DESCRIPTION_KEY;
  static const char* DEFAULT_KEY;
  static const char* VALUE_KEY;
  static const char* MIN_KEY;
  static const char* MAX_KEY;

  enum Type
  {
    BOOL,
    INT32,
    FLOAT32,
    FLOAT32_VEC3,
    FLOAT32_VEC4,
    ARRAY2D,
    STRING,
  };
};

#endif // vtkDynamicProperties_h
