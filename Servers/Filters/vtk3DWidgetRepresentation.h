/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtk3DWidgetRepresentation
// .SECTION Description
// vtk3DWidgetRepresentation is a vtkDataRepresentation subclass for 3D widgets
// and their representations. It makes it possible to add 3D widgets to
// vtkPVRenderView.

#ifndef __vtk3DWidgetRepresentation_h
#define __vtk3DWidgetRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkAbstractWidget;
class vtkPVRenderView;
class vtkWidgetRepresentation;

class VTK_EXPORT vtk3DWidgetRepresentation : public vtkDataRepresentation
{
public:
  static vtk3DWidgetRepresentation* New();
  vtkTypeMacro(vtk3DWidgetRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the widget.
  void SetWidget(vtkAbstractWidget*);
  vtkGetObjectMacro(Widget, vtkAbstractWidget);

  // Description:
  // Get/Set the representation.
  void SetRepresentation(vtkWidgetRepresentation*);
  vtkGetObjectMacro(Representation, vtkWidgetRepresentation);

  // Description:
  // Set to true to add the vtkWidgetRepresentation to the non-composited
  // renderer. false by default.
  vtkSetMacro(UseNonCompositedRenderer, bool);
  vtkGetMacro(UseNonCompositedRenderer, bool);
  vtkBooleanMacro(UseNonCompositedRenderer, bool);

  // Description:
  // Get/Set whether the widget is enabled.
  void SetEnabled(bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);

//BTX
protected:
  vtk3DWidgetRepresentation();
  ~vtk3DWidgetRepresentation();

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  void UpdateEnabled();

  bool Enabled;
  bool UseNonCompositedRenderer;
  vtkAbstractWidget* Widget;
  vtkWidgetRepresentation* Representation;
  vtkWeakPointer<vtkPVRenderView> View;
private:
  vtk3DWidgetRepresentation(const vtk3DWidgetRepresentation&); // Not implemented
  void operator=(const vtk3DWidgetRepresentation&); // Not implemented
//ETX
};

#endif
