/*=========================================================================

  Program:   ParaView
  Module:    vtkPolarAxesRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolarAxesRepresentation - representation for a polar-axes.
// .SECTION Description
// vtkPolarAxesRepresentation is a representation for the Polar-Axes that shows a
// bounding box with labels around any input dataset.

#ifndef vtkPolarAxesRepresentation_h
#define vtkPolarAxesRepresentation_h

#include "vtkRemotingViewsModule.h" //needed for exports

#include "vtkNew.h" // needed for vtkNew.
#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPolarAxesActor;
class vtkPolyData;
class vtkPVRenderView;
class vtkTextProperty;

class VTKREMOTINGVIEWS_EXPORT vtkPolarAxesRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkPolarAxesRepresentation* New();
  vtkTypeMacro(vtkPolarAxesRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  // Get the bounds of the data.
  vtkGetVector6Macro(DataBounds, double);

  // Description:
  // Get/Set custom bounds to use. When corresponding CustomBoundsActive is
  // true, the data bounds will be ignored for that direction and CustomBounds
  // will be used instead.
  vtkSetVector6Macro(CustomBounds, double);
  vtkGetVector6Macro(CustomBounds, double);

  // Description:
  // Get/Set whether to use custom bounds for a particular dimension.
  vtkSetVector3Macro(EnableCustomBounds, int);
  vtkGetVector3Macro(EnableCustomBounds, int);

  // Description:
  // Set the use of a custom range.
  vtkSetMacro(EnableCustomRange, bool);
  vtkGetMacro(EnableCustomRange, bool);

  // Description:
  // Get/Set custom range to use.
  // the data bounds will be ignored for that direction and CustomRange
  // will be used instead.
  vtkSetVector2Macro(CustomRange, double);
  vtkGetVector2Macro(CustomRange, double);

  // Description:
  // Set the actor color.
  virtual void SetPolarAxisColor(double r, double g, double b);
  virtual void SetPolarArcsColor(double r, double g, double b);
  virtual void SetSecondaryPolarArcsColor(double r, double g, double b);
  virtual void SetSecondaryRadialAxesColor(double r, double g, double b);
  virtual void SetLastRadialAxisColor(double r, double g, double b);
  virtual void SetPolarAxisTitleTextProperty(vtkTextProperty* prop);
  virtual void SetPolarAxisLabelTextProperty(vtkTextProperty* prop);
  virtual void SetLastRadialAxisTextProperty(vtkTextProperty* prop);
  virtual void SetSecondaryRadialAxesTextProperty(vtkTextProperty* prop);

  // Description:
  // This needs to be called on all instances of vtkPolarAxesRepresentation when
  // the input is modified.
  void MarkModified() override { this->Superclass::MarkModified(); }

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  // Description:
  // Set visibility of the representation.
  void SetVisibility(bool visible) override;

  // Description:
  // Set (forward) visibility of the parent (composite) representation.
  virtual void SetParentVisibility(bool visible);

  //***************************************************************************
  // Forwarded to internal vtkPolarAxesActor
  virtual void SetLog(bool active);
  virtual void SetNumberOfRadialAxes(vtkIdType val);
  virtual void SetNumberOfPolarAxisTicks(int val);
  virtual void SetAutoSubdividePolarAxis(bool active);
  virtual void SetMinimumRadius(double radius);
  virtual void SetMinimumAngle(double angle);
  virtual void SetMaximumAngle(double angle);
  virtual void SetSmallestVisiblePolarAngle(double angle);
  virtual void SetTickLocation(int location);
  virtual void SetRadialUnits(bool use);
  virtual void SetScreenSize(double size);
  virtual void SetPolarAxisTitle(const char* title);
  virtual void SetPolarLabelFormat(const char* format);
  virtual void SetExponentLocation(int location);
  virtual void SetRadialAngleFormat(const char* format);
  virtual void SetEnableDistanceLOD(int enable);
  virtual void SetDistanceLODThreshold(double val);
  virtual void SetEnableViewAngleLOD(int enable);
  virtual void SetViewAngleLODThreshold(double val);
  virtual void SetPolarAxisVisibility(int visible);
  virtual void SetDrawRadialGridlines(int draw);
  virtual void SetDrawPolarArcsGridlines(int draw);
  virtual void SetPolarTitleVisibility(int visible);
  virtual void SetRadialAxisTitleLocation(int location);
  virtual void SetPolarAxisTitleLocation(int location);
  virtual void SetPolarLabelVisibility(int visible);
  virtual void SetArcTicksOriginToPolarAxis(int use);
  virtual void SetRadialAxesOriginToPolarAxis(int use);
  virtual void SetPolarTickVisibility(int visible);
  virtual void SetAxisTickVisibility(int visible);
  virtual void SetAxisMinorTickVisibility(int visible);
  virtual void SetArcTickVisibility(int visible);
  virtual void SetArcMinorTickVisibility(int visible);
  virtual void SetArcMajorTickSize(double size);
  virtual void SetPolarAxisMajorTickSize(double size);
  virtual void SetLastRadialAxisMajorTickSize(double size);
  virtual void SetPolarAxisTickRatioSize(double size);
  virtual void SetLastAxisTickRatioSize(double size);
  virtual void SetArcTickRatioSize(double size);
  virtual void SetPolarAxisMajorTickThickness(double thickness);
  virtual void SetLastRadialAxisMajorTickThickness(double thickness);
  virtual void SetArcMajorTickThickness(double thickness);
  virtual void SetPolarAxisTickRatioThickness(double thickness);
  virtual void SetLastAxisTickRatioThickness(double thickness);
  virtual void SetArcTickRatioThickness(double thickness);
  virtual void SetDeltaAngleMajor(double delta);
  virtual void SetDeltaAngleMinor(double delta);
  virtual void SetRadialAxesVisibility(int visible);
  virtual void SetRadialTitleVisibility(int visible);
  virtual void SetPolarArcsVisibility(int visible);
  virtual void SetUse2DMode(int use);
  virtual void SetRatio(double ratio);
  virtual double GetDeltaRangeMinor();
  virtual double GetDeltaRangeMajor();

  // Description:
  // Set the renderer to use. Default is to use the
  // vtkPVRenderView::DEFAULT_RENDERER.
  vtkSetMacro(RendererType, int);
  vtkGetMacro(RendererType, int);

protected:
  vtkPolarAxesRepresentation();
  ~vtkPolarAxesRepresentation() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*) override;

  virtual void InitializeDataBoundsFromData(vtkDataObject* data);
  virtual void UpdateBounds();

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  bool AddToView(vtkView* view) override;

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  bool RemoveFromView(vtkView* view) override;

  vtkNew<vtkPolyData> OutlineGeometry;
  vtkWeakPointer<vtkPVRenderView> RenderView;
  vtkPolarAxesActor* PolarAxesActor;
  double Position[3];
  double Scale[3];
  double Orientation[3];
  double CustomBounds[6];
  int EnableCustomBounds[3];
  double CustomRange[2];
  bool EnableCustomRange;
  double DataBounds[6];
  int RendererType;
  vtkTimeStamp DataBoundsTime;
  bool ParentVisibility;

private:
  vtkPolarAxesRepresentation(const vtkPolarAxesRepresentation&) = delete;
  void operator=(const vtkPolarAxesRepresentation&) = delete;
};

#endif
