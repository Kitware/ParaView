/*=========================================================================

  Module:    vtkKWSurfaceMaterialPropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSurfaceMaterialPropertyWidget - widget to control the material property of a polygonal surface (vtkProperty)
// .SECTION Description

#ifndef __vtkKWSurfaceMaterialPropertyWidget_h
#define __vtkKWSurfaceMaterialPropertyWidget_h

#include "vtkKWMaterialPropertyWidget.h"

class vtkProperty;

class KWWidgets_EXPORT vtkKWSurfaceMaterialPropertyWidget : public vtkKWMaterialPropertyWidget
{
public:
  static vtkKWSurfaceMaterialPropertyWidget *New();
  vtkTypeRevisionMacro(vtkKWSurfaceMaterialPropertyWidget, vtkKWMaterialPropertyWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the volume property to edit with this widget
  virtual void SetProperty(vtkProperty *prop);
  vtkGetObjectMacro(Property, vtkProperty);

  // Description:
  // Refresh the interface given the value extracted from the current property.
  virtual void Update();

protected:

  vtkKWSurfaceMaterialPropertyWidget();
  ~vtkKWSurfaceMaterialPropertyWidget();
  
  vtkProperty *Property;

  // Description:
  // Update the property from the interface values or a preset
  // Return 1 if the property was modified, 0 otherwise
  virtual int UpdatePropertyFromInterface();
  virtual int UpdatePropertyFromPreset(const Preset *preset);

  // Description:
  // Send an event representing the state of the widget
  virtual void SendStateEvent(int event);

private:
  vtkKWSurfaceMaterialPropertyWidget(const vtkKWSurfaceMaterialPropertyWidget&);  //Not implemented
  void operator=(const vtkKWSurfaceMaterialPropertyWidget&);  //Not implemented
};

#endif
