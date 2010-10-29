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
// .NAME vtkCubeAxesRepresentation - representation for a cube-axes.
// .SECTION Description
// vtkCubeAxesRepresentation is a representation for the Cube-Axes that shows a
// bounding box with labels around any input dataset.

#ifndef __vtkCubeAxesRepresentation_h
#define __vtkCubeAxesRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkCubeAxesActor;
class vtkPVRenderView;

class VTK_EXPORT vtkCubeAxesRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkCubeAxesRepresentation* New();
  vtkTypeMacro(vtkCubeAxesRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the Position to transform the data bounds.
  vtkSetVector3Macro(Position, double);
  vtkGetVector3Macro(Position, double);

  // Description:
  // Get/Set the Orientation to transform the data bounds.
  vtkSetVector3Macro(Orientation, double);
  vtkGetVector3Macro(Orientation, double);

  // Description:
  // Get/Set the Scale to transform the data bounds.
  vtkSetVector3Macro(Scale, double);
  vtkGetVector3Macro(Scale, double);

  // Description:
  // Get/Set custom bounds to use. When corresponding CustomBoundsActive is
  // true, the data bounds will be ignored for that direction and CustomBounds
  // will be used instead.
  vtkSetVector6Macro(CustomBounds, double);
  vtkGetVector6Macro(CustomBounds, double);

  // Description:
  // Get/Set whether to use custom bounds for a particular dimension.
  vtkSetVector3Macro(CustomBoundsActive, int);
  vtkGetVector3Macro(CustomBoundsActive, int);

  // Description:
  // Set the actor color.
  void SetColor(double r, double g, double b);
  void SetColor(double rgb[3])
    { this->SetColor(rgb[0], rgb[1], rgb[2]); }

  // Description:
  // This needs to be called on all instances of vtkCubeAxesRepresentation when
  // the input is modified.
  virtual void MarkModified()
    { this->Superclass::MarkModified(); }

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  //***************************************************************************
  // Forwarded to internal vtkCubeAxesActor
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
  vtkCubeAxesRepresentation();
  ~vtkCubeAxesRepresentation();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  int RequestData(vtkInformation*,
    vtkInformationVector** inputVector, vtkInformationVector*);

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

  vtkWeakPointer<vtkPVRenderView> View;
  vtkTimeStamp BoundsUpdateTime;
  vtkCubeAxesActor* CubeAxesActor;
  double Position[3];
  double Scale[3];
  double Orientation[3];
  double CustomBounds[6];
  int CustomBoundsActive[3];
  double DataBounds[6];
private:
  vtkCubeAxesRepresentation(const vtkCubeAxesRepresentation&); // Not implemented
  void operator=(const vtkCubeAxesRepresentation&); // Not implemented
//ETX
};

#endif
