/*=========================================================================

  Program:   ParaView
  Module:    vtkCubeAxesRepresentation.h

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

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include "vtkSmartPointer.h" // needed for vtkWeakPointer.

class vtkCubeAxesActor;
class vtkPVRenderView;
class vtkPolyData;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkCubeAxesRepresentation : public vtkPVDataRepresentation
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
  // Get/Set the bounds of the data.
  vtkGetVector6Macro(DataBounds, double);

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
  vtkSetVector3Macro(CustomRangeActive, int);
  vtkGetVector3Macro(CustomRangeActive, int);

  // Description:
  // Get/Set whether to use original data bounds as range for a particular dimension.
  vtkSetVector3Macro(OriginalBoundsRangeActive, int);
  vtkGetVector3Macro(OriginalBoundsRangeActive, int);

  // Description:
  // Get/Set custom range to use. When corresponding CustomRangeActive is
  // true, the data bounds will be ignored for that direction and CustomBounds
  // will be used instead.
  vtkSetVector6Macro(CustomRange, double);
  vtkGetVector6Macro(CustomRange, double);

  // Description:
  // Set the actor color.
  virtual void SetColor(double r, double g, double b);
  virtual void SetColor(double rgb[3])
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

  // Description:
  // Allow user to specify custom XYZAxisRanges based on arbitrary number
  vtkSetMacro(UseDefaultXTitle,int);
  vtkGetMacro(UseDefaultXTitle,int);

  // Description:
  // Enable/Disable the usage of the FieldData to override the YTitle value.
  vtkSetMacro(UseDefaultYTitle,int);
  vtkGetMacro(UseDefaultYTitle,int);

  // Description:
  // Enable/Disable the usage of the FieldData to override the ZTitle value.
  vtkSetMacro(UseDefaultZTitle,int);
  vtkGetMacro(UseDefaultZTitle,int);

  // Description:
  // Set the user defined title that could be use it UseDefaultXTitle=0
  virtual void SetXTitle(const char* val);

  // Description:
  // Set the user defined title that could be use it UseDefaultYTitle=0
  virtual void SetYTitle(const char* val);

  // Description:
  // Set the user defined title that could be use it UseDefaultZTitle=0
  virtual void SetZTitle(const char* val);

  //***************************************************************************
  // Forwarded to internal vtkCubeAxesActor
  virtual void SetFlyMode(int val);
  virtual void SetInertia(int val);
  virtual void SetCornerOffset(double val);
  virtual void SetTickLocation(int val);

  virtual void SetXAxisVisibility(int val);
  virtual void SetXAxisTickVisibility(int val);
  virtual void SetXAxisMinorTickVisibility(int val);
  virtual void SetDrawXGridlines(int val);

  virtual void SetYAxisVisibility(int val);
  virtual void SetYAxisTickVisibility(int val);
  virtual void SetYAxisMinorTickVisibility(int val);
  virtual void SetDrawYGridlines(int val);

  virtual void SetZAxisVisibility(int val);
  virtual void SetZAxisTickVisibility(int val);
  virtual void SetZAxisMinorTickVisibility(int val);
  virtual void SetDrawZGridlines(int val);

  virtual void SetGridLineLocation(int val);

  virtual void SetUseOfAxesOrigin(int val);
  virtual void SetAxesOrigin(double valX, double valY, double valZ);
  virtual void SetAxesOrigin(double val[3]);

  virtual void SetXLabelFormat(const char* format);
  virtual void SetYLabelFormat(const char* format);
  virtual void SetZLabelFormat(const char* format);

  virtual void SetStickyAxes(int val);

  virtual void SetCenterStickyAxes(int val);
//BTX
protected:
  vtkCubeAxesRepresentation();
  ~vtkCubeAxesRepresentation();

  friend class vtkSliceFriendGeometryRepresentation;

  // Description:
  // Extract Field information from input if any and configure the underneath
  // cube axes if needed
  virtual void ConfigureCubeAxes(vtkDataObject* input);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation*,
    vtkInformationVector** inputVector, vtkInformationVector*);

  virtual void UpdateBounds();

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
  vtkSmartPointer<vtkPolyData> OutlineGeometry;
  vtkCubeAxesActor* CubeAxesActor;
  double Position[3];
  double Scale[3];
  double Orientation[3];
  double CustomBounds[6];
  int CustomBoundsActive[3];
  int OriginalBoundsRangeActive[3];
  double CustomRange[6];
  int CustomRangeActive[3];
  double DataBounds[6];
  bool UseOrientedBounds;
  int UseDefaultXTitle;
  int UseDefaultYTitle;
  int UseDefaultZTitle;
  char* UserXTitle;
  char* UserYTitle;
  char* UserZTitle;

  vtkSetStringMacro(UserXTitle);
  vtkSetStringMacro(UserYTitle);
  vtkSetStringMacro(UserZTitle);
private:
  vtkCubeAxesRepresentation(const vtkCubeAxesRepresentation&); // Not implemented
  void operator=(const vtkCubeAxesRepresentation&); // Not implemented
//ETX
};

#endif
