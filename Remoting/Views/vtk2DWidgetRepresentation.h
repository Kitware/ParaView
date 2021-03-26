/*=========================================================================

  Program:   ParaView
  Module:    vtk2DWidgetRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtk2DWidgetRepresentation_h
#define vtk2DWidgetRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkWeakPointer.h"

class vtkContextItem;
class vtkPVContextView;

/**
 * @class   vtk2DWidgetRepresentation
 *
 * vtk2DWidgetRepresentation is a vtkDataRepresentation subclass for 2D widgets
 * and their representations. It makes it possible to add 2D widgets to
 * vtkPVContextView.
 */

class VTKREMOTINGVIEWS_EXPORT vtk2DWidgetRepresentation : public vtkDataRepresentation
{
public:
  static vtk2DWidgetRepresentation* New();
  vtkTypeMacro(vtk2DWidgetRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the representation.
   */
  void SetContextItem(vtkContextItem*);
  vtkGetObjectMacro(ContextItem, vtkContextItem);
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
  vtk2DWidgetRepresentation();
  ~vtk2DWidgetRepresentation() override;

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

  void OnContextItemModified();

  vtkContextItem* ContextItem;
  bool Enabled;
  vtkWeakPointer<vtkPVContextView> View;

private:
  vtk2DWidgetRepresentation(const vtk2DWidgetRepresentation&) = delete;
  void operator=(const vtk2DWidgetRepresentation&) = delete;

  unsigned long ObserverTag;
};

#endif
