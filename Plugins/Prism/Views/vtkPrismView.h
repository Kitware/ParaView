// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPrismView
 * @brief   View for Prism representations.
 */
#ifndef vtkPVPrismView_h
#define vtkPVPrismView_h

#include "vtkPVRenderView.h"
#include "vtkPrismViewsModule.h" // needed for exports

class vtkPVLODActor;

class VTKPRISMVIEWS_EXPORT vtkPrismView : public vtkPVRenderView
{
public:
  static vtkPrismView* New();
  vtkTypeMacro(vtkPrismView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the x axis name.
   */
  vtkSetStringMacro(XAxisName);
  vtkGetStringMacro(XAxisName);
  ///@}

  ///@{
  /**
   * Set/Get the y axis name.
   */
  vtkSetStringMacro(YAxisName);
  vtkGetStringMacro(YAxisName);
  ///@}

  ///@{
  /**
   * Set/Get the z axis name.
   */
  vtkSetStringMacro(ZAxisName);
  vtkGetStringMacro(ZAxisName);
  ///@}

  ///@{
  /**
   * Set/Get If Threshold is enabled or not.
   */
  vtkSetMacro(EnableThresholding, bool);
  vtkBooleanMacro(EnableThresholding, bool);
  vtkGetMacro(EnableThresholding, bool);
  ///@}

  ///@{
  /**
   * Set/get the upper and lower thresholds for X axis.
   * The default values are set to +infinity and -infinity, respectively.
   */
  vtkSetMacro(UpperThresholdX, double);
  vtkSetMacro(LowerThresholdX, double);
  vtkGetMacro(UpperThresholdX, double);
  vtkGetMacro(LowerThresholdX, double);
  ///@}

  ///@{
  /**
   * Set/get the upper and lower thresholds for Y axis.
   * The default values are set to +infinity and -infinity, respectively.
   */
  vtkSetMacro(UpperThresholdY, double);
  vtkSetMacro(LowerThresholdY, double);
  vtkGetMacro(UpperThresholdY, double);
  vtkGetMacro(LowerThresholdY, double);
  ///@}

  ///@{
  /**
   * Set/get the upper and lower thresholds for X axis.
   * The default values are set to +infinity and -infinity, respectively.
   */
  vtkSetMacro(UpperThresholdZ, double);
  vtkSetMacro(LowerThresholdZ, double);
  vtkGetMacro(UpperThresholdZ, double);
  vtkGetMacro(LowerThresholdZ, double);
  ///@}

  ///@{
  /**
   * Set/Get If X axis will be Log Scaled
   */
  vtkSetMacro(LogScaleX, bool);
  vtkBooleanMacro(LogScaleX, bool);
  vtkGetMacro(LogScaleX, bool);
  ///@}

  ///@{
  /**
   * Set/Get If Y axis will be Log Scaled
   */
  vtkSetMacro(LogScaleY, bool);
  vtkBooleanMacro(LogScaleY, bool);
  vtkGetMacro(LogScaleY, bool);
  ///@}

  ///@{
  /**
   * Set/Get If Z axis will be Log Scaled
   */
  vtkSetMacro(LogScaleZ, bool);
  vtkBooleanMacro(LogScaleZ, bool);
  vtkGetMacro(LogScaleZ, bool);
  ///@}

  ///@{
  /**
   * Set/Get the aspect ratio.
   *
   * The default values are set to 1, 1, 1.
   */
  vtkSetVector3Macro(AspectRatio, double);
  vtkGetVector3Macro(AspectRatio, double);
  ///@}

  /**
   * AboutToRenderOnLocalProcess is used to set the axis titles and transformation.
   */
  void AboutToRenderOnLocalProcess(bool interactive) override;

  /**
   * This is a Update-Data pass to force representation that are non-simulation data
   * to get their bounds.
   */
  static vtkInformationRequestKey* REQUEST_BOUNDS();

  /**
   * Request Data mode which will be used by the representations.
   */
  enum class RequestDataModes
  {
    REQUEST_BOUNDS,
    REQUEST_DATA,
  };

  ///@{
  /**
   * Set/Get the mode for the REQUEST_DATA() pass of the representations.
   */
  virtual RequestDataModes GetRequestDataMode()
  {
    vtkDebugMacro(<< " returning RequestDataMode of " << (int)this->RequestDataMode);
    return this->RequestDataMode;
  }
  ///@}

  ///@{
  /**
   * Get the prism bounds.
   */
  vtkGetVector6Macro(PrismBounds, double);
  ///@}

  /**
   * Overridden to ensure that the representations are updated in the correct order.
   */
  void Update() override;

  ///@{
  /**
   * Compute/Get the prism bounds.
   */
  vtkSetMacro(EnableNonSimulationDataSelection, bool);
  vtkGetMacro(EnableNonSimulationDataSelection, bool);
  vtkBooleanMacro(EnableNonSimulationDataSelection, bool);
  ///@}

  /**
   * Prepare for selection.
   * Returns false if it is currently generating a selection.
   */
  bool PrepareSelect(int fieldAssociation, const char* array = nullptr) override;

  /**
   * Post process after selection.
   */
  void PostSelect(vtkSelection* sel, const char* array = nullptr) override;

protected:
  vtkPrismView();
  ~vtkPrismView() override;

  void AllReduceString(const std::string& axisNameSource, std::string& axisNameDestination);

  void SynchronizeGeometryBounds() override;

  bool EnableThresholding = false;
  double UpperThresholdX = VTK_DOUBLE_MIN;
  double LowerThresholdX = VTK_DOUBLE_MAX;
  double UpperThresholdY = VTK_DOUBLE_MIN;
  double LowerThresholdY = VTK_DOUBLE_MAX;
  double UpperThresholdZ = VTK_DOUBLE_MIN;
  double LowerThresholdZ = VTK_DOUBLE_MAX;

  bool LogScaleX = false;
  bool LogScaleY = false;
  bool LogScaleZ = false;

  char* XAxisName = nullptr;
  char* YAxisName = nullptr;
  char* ZAxisName = nullptr;

  double AspectRatio[3] = { 1.0, 1.0, 1.0 };

  RequestDataModes RequestDataMode = RequestDataModes::REQUEST_BOUNDS;
  vtkBoundingBox PrismBoundsBBox;
  double PrismBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

  bool EnableNonSimulationDataSelection = false;

  std::vector<vtkPVLODActor*> NonSimulationPropsToHideForSelection;

private:
  vtkPrismView(const vtkPrismView&) = delete;
  void operator=(const vtkPrismView&) = delete;
};

#endif // vtkPVPrismView_h
