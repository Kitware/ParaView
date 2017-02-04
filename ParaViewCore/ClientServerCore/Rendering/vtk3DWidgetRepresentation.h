/*=========================================================================

  Program:   ParaView
  Module:    vtk3DWidgetRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtk3DWidgetRepresentation
 *
 * vtk3DWidgetRepresentation is a vtkDataRepresentation subclass for 3D widgets
 * and their representations. It makes it possible to add 3D widgets to
 * vtkPVRenderView.
*/

#ifndef vtk3DWidgetRepresentation_h
#define vtk3DWidgetRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkWeakPointer.h"                       // needed for vtkWeakPointer.

class vtkAbstractWidget;
class vtkPVRenderView;
class vtkWidgetRepresentation;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtk3DWidgetRepresentation : public vtkDataRepresentation
{
public:
  static vtk3DWidgetRepresentation* New();
  vtkTypeMacro(vtk3DWidgetRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the widget.
   */
  void SetWidget(vtkAbstractWidget*);
  vtkGetObjectMacro(Widget, vtkAbstractWidget);
  //@}

  //@{
  /**
   * Get/Set the representation.
   */
  void SetRepresentation(vtkWidgetRepresentation*);
  vtkGetObjectMacro(Representation, vtkWidgetRepresentation);
  //@}

  //@{
  /**
   * Set to true to add the vtkWidgetRepresentation to the non-composited
   * renderer. false by default.
   */
  vtkSetMacro(UseNonCompositedRenderer, bool);
  vtkGetMacro(UseNonCompositedRenderer, bool);
  vtkBooleanMacro(UseNonCompositedRenderer, bool);
  //@}

  //@{
  /**
   * Get/Set whether the widget is enabled.
   */
  void SetEnabled(bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);
  //@}

protected:
  vtk3DWidgetRepresentation();
  ~vtk3DWidgetRepresentation();

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  /**
   * Updates 'Enabled' on this->Widget.
   */
  void UpdateEnabled();

  /**
   * Callback whenever the representation is modified. We call UpdateEnabled()
   * to ensure that the widget is not left enabled when the representation is
   * hidden.
   */
  void OnRepresentationModified();

  /**
   * Callback whenever the view is modified. If the view's interactor has
   * changed, we will pass that to the vtkAbstractWidget instance and then call
   * UpdateEnabled().
   */
  void OnViewModified();

  bool Enabled;
  bool UseNonCompositedRenderer;
  vtkAbstractWidget* Widget;
  vtkWidgetRepresentation* Representation;
  vtkWeakPointer<vtkPVRenderView> View;

private:
  vtk3DWidgetRepresentation(const vtk3DWidgetRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtk3DWidgetRepresentation&) VTK_DELETE_FUNCTION;
  unsigned long RepresentationObserverTag;
  unsigned long ViewObserverTag;
};

#endif
