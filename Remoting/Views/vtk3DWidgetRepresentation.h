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
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkWeakPointer.h"         // needed for vtkWeakPointer.

class vtkAbstractWidget;
class vtkPVRenderView;
class vtkWidgetRepresentation;

class VTKREMOTINGVIEWS_EXPORT vtk3DWidgetRepresentation : public vtkDataRepresentation
{
public:
  static vtk3DWidgetRepresentation* New();
  vtkTypeMacro(vtk3DWidgetRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  //@{
  /**
   * These are needed to support BoxWidget use-case where we want to support
   * specification of the box using global transform or relative to some
   * reference bounds. Since this may be applicable to other 3D widgets that
   * have similar requirements, we add this ability to vtk3DWidgetRepresentation
   * itself. All this does it based on the state of UseReferenceBounds,
   * `vtkWidgetRepresentation::PlaceWidget` is called using either the
   * `ReferenceBounds` or the bounds passed to `PlaceWidget`.
   */
  void SetReferenceBounds(const double bds[6]);
  void PlaceWidget(const double bds[6]);
  void SetUseReferenceBounds(bool);
  //@}

protected:
  vtk3DWidgetRepresentation();
  ~vtk3DWidgetRepresentation() override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

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
  vtk3DWidgetRepresentation(const vtk3DWidgetRepresentation&) = delete;
  void operator=(const vtk3DWidgetRepresentation&) = delete;
  unsigned long RepresentationObserverTag;
  unsigned long ViewObserverTag;

  double ReferenceBounds[6];
  bool UseReferenceBounds;
  double PlaceWidgetBounds[6];

  void PlaceWidget();
};

#endif
