// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVRenderViewSettings
 * @brief   singleton used to keep track of options for
 * vtkPVRenderView.
 *
 * vtkPVRenderViewSettings is a singleton used to keep track of selections for
 * various configurable parameters used by vtkPVRenderView.
 * All class to vtkPVRenderViewSettings::New() returns a reference to the
 * singleton instance.
 */

#ifndef vtkPVRenderViewSettings_h
#define vtkPVRenderViewSettings_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer

class VTKREMOTINGVIEWS_EXPORT vtkPVRenderViewSettings : public vtkObject
{
public:
  static vtkPVRenderViewSettings* New();
  vtkTypeMacro(vtkPVRenderViewSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Access the singleton.
   */
  static vtkPVRenderViewSettings* GetInstance();

  enum
  {
    DO_NOTHING = 0,
    OFFSET_FACES = 1,
    OFFSET_LINES_AND_VERTS = 2,
    ZSHIFT = 3
  };

  ///@{
  /**
   * vtkMapper settings.
   */
  void SetResolveCoincidentTopology(int mode);
  void SetPolygonOffsetParameters(double factor, double units);
  void SetLineOffsetParameters(double factor, double units);
  void SetPointOffsetParameter(double units);
  void SetZShift(double a);
  ///@}

  ///@{
  /**
   * Set the number of cells (in millions) when the representations show try to
   * use outline by default.
   */
  vtkSetMacro(OutlineThreshold, vtkIdType);
  vtkGetMacro(OutlineThreshold, vtkIdType);
  ///@}

  ///@{
  /**
   * When rendering a volume, this option simulates light coming from both its direction
   * and its opposite for all lights in the scene. It is enabled by default.
   */
  vtkSetMacro(TwoSidedLighting, bool);
  vtkGetMacro(TwoSidedLighting, bool);
  ///@}

  enum
  {
    AUTOMATIC = 0,
    ALWAYS_2D = 1,
    ALWAYS_3D = 2
  };
  ///@{
  /**
   * Set the default interaction mode.
   */
  vtkSetMacro(DefaultInteractionMode, int);
  vtkGetMacro(DefaultInteractionMode, int);
  ///@}

  ///@{
  /**
   * Set the radius in pixels to include for finding the closet point when
   * selecting a point on render view. This only after single point selections
   * i.e. when selecting a regions this radius is not respected.
   */
  vtkSetMacro(PointPickingRadius, int);
  vtkGetMacro(PointPickingRadius, int);
  ///@}

  ///@{
  /**
   * EXPERIMENTAL: Add ability to disable IceT.
   */
  vtkSetMacro(DisableIceT, bool);
  vtkGetMacro(DisableIceT, bool);
  ///@}

  ///@{
  /**
   * Enable fast preselection. When enabled, the preselection is computed using
   * the visible geometry. This avoids a call to the extract selection filter each time
   * the mouse moves, improving a lot the preselection highlight display, especially for
   * big datasets.
   */
  vtkSetMacro(EnableFastPreselection, bool);
  vtkGetMacro(EnableFastPreselection, bool);
  ///@}

  ///@{
  /**
   * When enabled and growing selection, remove the initial selection seed.
   * Default is false.
   */
  vtkSetMacro(GrowSelectionRemoveSeed, bool);
  vtkGetMacro(GrowSelectionRemoveSeed, bool);
  ///@}

  ///@{
  /**
   * When enabled and growing selection, remove the intermediate layers.
   * Default is false.
   */
  vtkSetMacro(GrowSelectionRemoveIntermediateLayers, bool);
  vtkGetMacro(GrowSelectionRemoveIntermediateLayers, bool);
  ///@}

  ///@{
  /**
   * Used by vtkPVRenderView and other views to determine background color.
   */
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);
  vtkSetVector3Macro(Background2Color, double);
  vtkGetVector3Macro(Background2Color, double);
  vtkSetMacro(BackgroundColorMode, int);
  vtkGetMacro(BackgroundColorMode, int);
  ///@}

  ///@{
  /**
   * OffsetRatio is used to add a zoom offset when using Reset Camera Closest and
   * Zoom Closest To Data.
   *
   * Default is 0.9.
   */
  vtkSetMacro(ZoomClosestOffsetRatio, double);
  vtkGetMacro(ZoomClosestOffsetRatio, double);
  ///@}

protected:
  vtkPVRenderViewSettings();
  ~vtkPVRenderViewSettings() override;

  int DefaultInteractionMode;
  vtkIdType OutlineThreshold;
  bool TwoSidedLighting = true;
  int PointPickingRadius;
  bool DisableIceT;
  bool EnableFastPreselection;
  bool GrowSelectionRemoveSeed = false;
  bool GrowSelectionRemoveIntermediateLayers = false;

  double BackgroundColor[3];
  double Background2Color[3];
  int BackgroundColorMode;
  double ZoomClosestOffsetRatio;

private:
  vtkPVRenderViewSettings(const vtkPVRenderViewSettings&) = delete;
  void operator=(const vtkPVRenderViewSettings&) = delete;

  static vtkSmartPointer<vtkPVRenderViewSettings> Instance;
};

#endif
