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
// .NAME vtkPrismCubeAxesRepresentation - representation for a cube-axes.
// .SECTION Description
// vtkPrismCubeAxesRepresentation is a representation for the Cube-Axes that shows a
// bounding box with labels around any input dataset.

#ifndef __vtkPrismCubeAxesRepresentation_h
#define __vtkPrismCubeAxesRepresentation_h

#include "vtkCubeAxesRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPrismCubeAxesActor;
class vtkPVRenderView;

class VTK_EXPORT vtkPrismCubeAxesRepresentation : public vtkCubeAxesRepresentation
{
public:
  static vtkPrismCubeAxesRepresentation* New();
  vtkTypeMacro(vtkPrismCubeAxesRepresentation, vtkCubeAxesRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set the actor color.
  void SetColor(double r, double g, double b);



  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  //***************************************************************************
  // Forwarded to internal vtkPrismCubeAxesActor
  void SetLabelRanges(
    double a, double b, double c, double d, double e, double f);
  void SetFlyMode(int val);
  void SetInertia(int val);
  void SetCornerOffset(double val);
  void SetTickLocation(int val);

  void SetXTitle(const char* val);
  void SetXAxisVisibility(int val);
  void SetXAxisTickVisibility(int val);
  void SetXAxisMinorTickVisibility(int val);
  void SetDrawXGridlines(int val);

  void SetYAxisVisibility(int val);
  void SetYTitle(const char* val);
  void SetYAxisTickVisibility(int val);
  void SetYAxisMinorTickVisibility(int val);
  void SetDrawYGridlines(int val);

  void SetZAxisVisibility(int val);
  void SetZTitle(const char* val);
  void SetZAxisTickVisibility(int val);
  void SetZAxisMinorTickVisibility(int val);
  void SetDrawZGridlines(int val);
//BTX
protected:
  vtkPrismCubeAxesRepresentation();
  ~vtkPrismCubeAxesRepresentation();


  void UpdateBounds();

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

  vtkPrismCubeAxesActor* PrismCubeAxesActor;
private:
  vtkPrismCubeAxesRepresentation(const vtkPrismCubeAxesRepresentation&); // Not implemented
  void operator=(const vtkPrismCubeAxesRepresentation&); // Not implemented
//ETX
};

#endif
