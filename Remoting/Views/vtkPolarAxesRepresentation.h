// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPolarAxesRepresentation
 * @brief representation for a polar-axes.
 * vtkPolarAxesRepresentation is a representation for the Polar-Axes that shows a
 * bounding box with labels around any input dataset.
 */

#ifndef vtkPolarAxesRepresentation_h
#define vtkPolarAxesRepresentation_h

#include "vtkRemotingViewsModule.h" //needed for exports

#include "vtkNew.h" // needed for vtkNew.
#include "vtkPVDataRepresentation.h"
#include "vtkPVRenderView.h"        // needed for renderer enum
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_13_0
#include "vtkWeakPointer.h"         // needed for vtkWeakPointer.

class vtkPolarAxesActor;
class vtkPolyData;
class vtkTextProperty;

class VTKREMOTINGVIEWS_EXPORT vtkPolarAxesRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkPolarAxesRepresentation* New();
  vtkTypeMacro(vtkPolarAxesRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the Position to transform the data bounds.
   */
  vtkSetVector3Macro(Position, double);
  vtkGetVector3Macro(Position, double);
  ///@}

  ///@{
  /**
   * Get/Set the Orientation to transform the data bounds.
   */
  vtkSetVector3Macro(Orientation, double);
  vtkGetVector3Macro(Orientation, double);
  ///@}

  ///@{
  /**
   * Get/Set the Scale to transform the data bounds.
   */
  vtkSetVector3Macro(Scale, double);
  vtkGetVector3Macro(Scale, double);
  ///@}

  /**
   * Get the bounds of the data.
   */
  vtkGetVector6Macro(DataBounds, double);

  ///@{
  /**
   * Get/Set custom bounds to use. When corresponding CustomBoundsActive is
   * true, the data bounds will be ignored for that direction and CustomBounds
   * will be used instead.
   */
  vtkSetVector6Macro(CustomBounds, double);
  vtkGetVector6Macro(CustomBounds, double);
  ///@}

  ///@{
  /**
   * Get/Set whether to use custom bounds for a particular dimension.
   */
  vtkSetVector3Macro(EnableCustomBounds, int);
  vtkGetVector3Macro(EnableCustomBounds, int);
  ///@}

  ///@{
  /**
   * Get/Set the use of a custom range.
   */
  vtkSetMacro(EnableCustomRange, bool);
  vtkGetMacro(EnableCustomRange, bool);
  ///@}

  ///@{
  /**
   * Get/Set custom range to use.
   * The data bounds will be ignored for that direction and CustomRange
   * will be used instead.
   */
  vtkSetVector2Macro(CustomRange, double);
  vtkGetVector2Macro(CustomRange, double);
  ///@}

  ///@{
  /**
   * Get/Set the use of automatically placed pole (origin of the axes).
   * If on, pole is placed at the center of the bounding box.
   */
  vtkSetMacro(EnableAutoPole, bool);
  vtkGetMacro(EnableAutoPole, bool);
  ///@}

  ///@{
  /**
   * Get/Set the use of custom min/max angles.
   * If off, min/max angles are computed relatively to pole position.
   */
  vtkSetMacro(EnableCustomAngle, bool);
  vtkGetMacro(EnableCustomAngle, bool);
  ///@}

  ///@{
  /**
   * Get/Set the custom min/max angles when EnableCustomAngle is On.
   */
  vtkSetMacro(MinAngle, double);
  vtkGetMacro(MinAngle, double);
  vtkSetMacro(MaxAngle, double);
  vtkGetMacro(MaxAngle, double);
  ///@}

  ///@{
  /**
   * Get/Set the use of custom min radius.
   * If off, min radius is computed relatively to pole position.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use EnableCustomMinRadius instead")
  virtual void SetEnableCustomRadius(bool enabled);
  PARAVIEW_DEPRECATED_IN_5_13_0("Use EnableCustomMinRadius instead")
  virtual bool GetEnableCustomRadius();
  void SetEnableCustomMinRadius(bool enabled);
  vtkGetMacro(EnableCustomMinRadius, double);
  ///@}

  ///@{
  /**
   * Get/Set the custom min radius when EnableCustomMinRadius is On.
   * Default to 0.
   */
  vtkSetMacro(MinRadius, double);
  vtkGetMacro(MinRadius, double);
  ///@}

  ///@{
  /**
   * Get/Set the use of custom min radius.
   * If off, max radius is computed relatively to pole position.
   */
  vtkSetMacro(EnableCustomMaxRadius, bool);
  vtkGetMacro(EnableCustomMaxRadius, bool);
  ///@}

  ///@{
  /**
   * Get/Set the custom min radius when EnableCustomMaxRadius is On.
   * Default to 1.
   */
  vtkSetMacro(MaxRadius, double);
  vtkGetMacro(MaxRadius, double);
  ///@}

  ///@{
  /**
   * Set the actor color.
   */
  vtkGetMacro(EnableOverallColor, bool);
  virtual void SetEnableOverallColor(bool enable);
  virtual void SetOverallColor(double r, double g, double b);
  virtual void SetPolarAxisColor(double r, double g, double b);
  virtual void SetPolarArcsColor(double r, double g, double b);
  virtual void SetSecondaryPolarArcsColor(double r, double g, double b);
  virtual void SetSecondaryRadialAxesColor(double r, double g, double b);
  virtual void SetLastRadialAxisColor(double r, double g, double b);
  virtual void SetPolarAxisTitleTextProperty(vtkTextProperty* prop);
  virtual void SetPolarAxisLabelTextProperty(vtkTextProperty* prop);
  virtual void SetLastRadialAxisTextProperty(vtkTextProperty* prop);
  virtual void SetSecondaryRadialAxesTextProperty(vtkTextProperty* prop);
  ///@}

  /**
   * This needs to be called on all instances of vtkPolarAxesRepresentation when
   * the input is modified.
   */
  void MarkModified() override { this->Superclass::MarkModified(); }

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g. PrepareForRendering
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

  /**
   * Set (forward) visibility of the parent (composite) representation.
   */
  virtual void SetParentVisibility(bool visible);

  //***************************************************************************
  ///@{
  /**
   * Forwarded to internal vtkPolarAxesActor
   */
  virtual void SetLog(bool active);
  virtual void SetNumberOfRadialAxes(vtkIdType val);
  virtual void SetNumberOfPolarAxes(vtkIdType val);
  virtual void SetDeltaAngleRadialAxes(double angle);
  virtual void SetDeltaRangePolarAxes(double range);
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
  virtual void SetRadialTitleOffset(double offsetX, double offsetY);
  virtual void SetPolarTitleOffset(double offsetX, double offsetY);
  virtual void SetPolarLabelOffset(double offsetY);
  virtual void SetPolarExponentOffset(double offsetY);
  virtual void SetPolarLabelVisibility(int visible);
  virtual void SetArcTicksOriginToPolarAxis(int use);
  virtual void SetRadialAxesOriginToPolarAxis(int use);
  virtual void SetPolarTickVisibility(int visible);
  virtual void SetAxisTickVisibility(int visible);
  virtual void SetAxisMinorTickVisibility(int visible);
  virtual void SetAxisTickMatchesPolarAxes(int enable);
  virtual void SetArcTickVisibility(int visible);
  virtual void SetArcMinorTickVisibility(int visible);
  virtual void SetArcTickMatchesRadialAxes(int enable);
  virtual void SetTickRatioRadiusSize(double ratio);
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
  virtual void SetPolarArcResolutionPerDegree(double resolution);
  virtual void SetDeltaRangeMinor(double delta);
  virtual void SetDeltaRangeMajor(double delta);
  ///@}

  ///@{
  /**
   * Set the renderer to use. Default is to use the vtkPVRenderView::DEFAULT_RENDERER.
   */
  vtkSetMacro(RendererType, int);
  vtkGetMacro(RendererType, int);
  ///@}

protected:
  vtkPolarAxesRepresentation();
  ~vtkPolarAxesRepresentation() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*) override;

  virtual void InitializeDataBoundsFromData(vtkDataObject* data);
  virtual void UpdateBounds();

  /**
   * Adds the representation to the view. This is called from vtkView::AddRepresentation().
   * Returns true if the addition succeeds.
   *
   * Subclasses should override this method.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view. This is called from vtkView::RemoveRepresentation().
   * Returns true if the removal succeeds.
   *
   * Subclasses should override this method.
   */
  bool RemoveFromView(vtkView* view) override;

  vtkNew<vtkPolyData> OutlineGeometry;
  vtkNew<vtkPolarAxesActor> PolarAxesActor;
  vtkWeakPointer<vtkPVRenderView> RenderView;
  double Position[3] = { 0.0 };
  double Scale[3] = { 1.0 };
  double Orientation[3] = { 0.0 };
  double CustomBounds[6] = { 0.0, 1.0, 0.0, 1.0, 0.0, 1.0 };
  int EnableCustomBounds[3] = { 0 };
  double CustomRange[2] = { 0.0, 1.0 };
  bool EnableCustomRange = false;
  bool EnableAutoPole = true;
  bool EnableCustomAngle = true;
  double MinAngle = 0.0;
  double MaxAngle = 90.0;
  PARAVIEW_DEPRECATED_IN_5_13_0("Use Get/Set EnableCustomMinRadius methods instead")
  bool EnableCustomRadius = true;
  double MinRadius = 0.0;
  bool EnableOverallColor = true;
  double OverallColor[3] = { 1.0 };
  double PolarAxisColor[3] = { 1.0 };
  double PolarArcsColor[3] = { 1.0 };
  double SecondaryPolarArcsColor[3] = { 1.0 };
  double SecondaryRadialAxesColor[3] = { 1.0 };
  double LastRadialAxisColor[3] = { 1.0 };
  double DataBounds[6] = { 0.0 };
  int RendererType = vtkPVRenderView::DEFAULT_RENDERER;
  bool ParentVisibility = true;
  vtkTimeStamp DataBoundsTime;

private:
  vtkPolarAxesRepresentation(const vtkPolarAxesRepresentation&) = delete;
  void operator=(const vtkPolarAxesRepresentation&) = delete;

  bool EnableCustomMinRadius = true;
  bool EnableCustomMaxRadius = false;
  double MaxRadius = 1.0;
};

#endif
