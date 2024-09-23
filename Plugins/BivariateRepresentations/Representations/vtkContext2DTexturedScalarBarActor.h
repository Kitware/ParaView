// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/** @class   vtkContext2DTexturedScalarBarActor
 *  @brief   Custom scalar bar actor for ParaView that uses the Charts API.
 *
 * .SECTION Description
 *
 * vtkContext2DTexturedScalarBarActor is a custom scalar bar actor that uses the
 * Charts API for drawing calls. It can be used to display a texture in a 2D
 * square and both vertical and horizontal axis. Thus, it does not have an
 * orientation like the standard vtkContext2DScalarBarActor.
 *
 * Unlike the vtkContext2DScalarBarActor, this scalar bar does not use any
 * Lookup table to display colors. It doesn't have an orientation as well.
 *
 * You must use the following methods to set it up:
 * - `SetTexture` to set the displayed square texture
 * - `SetFirstRange` and `SetSecondRange` to set the horizontal and vertical axes
 *   ranges respectively (all values in between are then generated automatically)
 * - `SetTitle` and `SetTitle2` to set the horizontal and vertical axes respectively.
 *
 * @sa vtkContext2DScalarBarActor
 */

#ifndef vtkContext2DTexturedScalarBarActor_h
#define vtkContext2DTexturedScalarBarActor_h

#include "vtkBivariateRepresentationsModule.h" // for export macro
#include "vtkScalarBarActor.h"

#include "vtkCoordinate.h"   // for Position and Position 2
#include "vtkRect.h"         // for functions that return vtkRects
#include "vtkSmartPointer.h" // for smart pointers

#include "vtkTexture.h"

class vtkAxis;
class vtkColorLegend;
class vtkColorTransferFunctionItem;
class vtkContextActor;
class vtkContext2D;
class vtkContextScene;
class vtkDoubleArray;
class vtkImageData;
class vtkPoints2D;
class vtkScalarsToColors;
class vtkTextProperty;
class vtkViewport;

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkContext2DTexturedScalarBarActor
  : public vtkScalarBarActor
{
public:
  vtkTypeMacro(vtkContext2DTexturedScalarBarActor, vtkScalarBarActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkContext2DTexturedScalarBarActor* New();

  ///@{
  /**
   * Set the title justification. Valid values are VTK_TEXT_LEFT,
   * VTK_TEXT_CENTERED, and VTK_TEXT_RIGHT.
   */
  vtkGetMacro(TitleJustification, int);
  vtkSetClampMacro(TitleJustification, int, VTK_TEXT_LEFT, VTK_TEXT_RIGHT);
  ///@}

  ///@{
  /**
   * Set the scalar bar sides length, specified in normalized viewport coordinates.
   * This means the value is the fractional span of the viewport's width in the range [0, 1].
   */
  vtkSetClampMacro(ScalarBarLength, double, 0, 1);
  vtkGetMacro(ScalarBarLength, double);
  ///@}

  ///@{
  /**
   * Enable drawing an outline around the scalar bar.
   */
  vtkSetMacro(DrawScalarBarOutline, bool);
  vtkGetMacro(DrawScalarBarOutline, bool);
  vtkBooleanMacro(DrawScalarBarOutline, bool);
  ///@}

  ///@{
  /**
   * Set the RGB color of the scalar bar outline.
   */
  vtkSetVector3Macro(ScalarBarOutlineColor, double);
  vtkGetVector3Macro(ScalarBarOutlineColor, double);
  ///@}

  ///@{
  /**
   * Set the thickness of the scalar bar outline.
   */
  vtkSetClampMacro(ScalarBarOutlineThickness, int, 0, VTK_INT_MAX);
  vtkGetMacro(ScalarBarOutlineThickness, int);
  ///@}

  ///@{
  /**
   * Set color of background to draw behind the color bar. First three components,
   * specify RGB color components, opacity is the fourth element.
   */
  ///@}
  vtkSetVector4Macro(BackgroundColor, double);
  vtkGetVector4Macro(BackgroundColor, double);

  ///@{
  /**
   * Set the padding to add to the background rectangle past the contents of
   * the color legend.
   */
  ///@}
  vtkSetClampMacro(BackgroundPadding, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(BackgroundPadding, double);

  ///@{
  /**
   * If true (the default), the printf format used for the labels will be
   * automatically generated to make the numbers best fit within the widget.  If
   * false, the LabelFormat ivar will be used.
   */
  vtkGetMacro(AutomaticLabelFormat, bool);
  vtkSetMacro(AutomaticLabelFormat, bool);
  vtkBooleanMacro(AutomaticLabelFormat, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to add range labels or not. These are labels that have
   * the minimum/maximum values of the scalar bar range.
   */
  vtkSetMacro(AddRangeLabels, bool);
  vtkGetMacro(AddRangeLabels, bool);
  ///@}

  ///@{
  /**
   * Set/get whether tick marks should be drawn.
   */
  vtkSetMacro(DrawTickMarks, bool);
  vtkGetMacro(DrawTickMarks, bool);
  ///@}

  ///@{
  /**
   * Printf format for range labels.
   */
  vtkSetMacro(RangeLabelFormat, std::string);
  vtkGetMacro(RangeLabelFormat, std::string);
  ///@}

  ///@{
  /**
   * Set number of custom labels.
   */
  void SetNumberOfCustomLabels(vtkIdType numLabels);
  vtkIdType GetNumberOfCustomLabels();
  ///@}

  /**
   * Set label for index.
   */
  void SetCustomLabel(vtkIdType index, double value);

  /**
   * We only render in the overlay for the context scene.
   */
  int RenderOverlay(vtkViewport* viewport) override;

  /**
   * Draw the scalar bar and annotation text to the screen.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

  /**
   * Responsible for actually drawing the scalar bar.
   */
  virtual bool Paint(vtkContext2D* painter);

  /**
   * Get the bounding rectangle of the scalar bar actor contents in display
   * coordinates.
   */
  vtkRectf GetBoundingRect();

  ///@{
  /**
   * Set/get the Logo source representation.
   * This representation is used to reprent the current 2D texture as a logo
   * in the current render view.
   */
  vtkSetSmartPointerMacro(Texture, vtkTexture);
  vtkGetSmartPointerMacro(Texture, vtkTexture);
  ///@}

  ///@{
  /**
   * Set/get the range of the first array used to draw the horizontal axis.
   */
  vtkSetVector2Macro(FirstRange, double);
  vtkGetVector2Macro(FirstRange, double);
  ///@}

  ///@{
  /**
   * Set/get the range of the second array used to draw the vertical axis.
   */
  vtkSetVector2Macro(SecondRange, double);
  vtkGetVector2Macro(SecondRange, double);
  ///@}

  ///@{
  /**
   * Set/Get the title of the second axis.
   */
  vtkSetMacro(Title2, std::string);
  vtkGetMacro(Title2, std::string);
  ///@}

  /**
   * Expected in the context 2D scalar bar actor API.
   * Always return 0 here since the scalar bar does not relies on the LUT to generate the
   * annotations.
   */
  int GetEstimatedNumberOfAnnotations() { return 0; }

protected:
  vtkContext2DTexturedScalarBarActor();
  ~vtkContext2DTexturedScalarBarActor() override;

private:
  vtkContext2DTexturedScalarBarActor(const vtkContext2DTexturedScalarBarActor&) = delete;
  void operator=(const vtkContext2DTexturedScalarBarActor&) = delete;

  /**
   * Get the size of the color bar area as determined by the position
   * coordinates. The painter parameter is needed to calculate the thickness
   * of the scalar bar in terms of the selected font size.
   */
  double GetSize();

  /**
   * Copy appropriate text properties to the axis text properties.
   */
  void UpdateTextProperties();

  /**
   * Paint the color bar itself.
   */
  void PaintColorBar(vtkContext2D* painter, double size);

  /**
   * Paint the axis and label positions.
   */
  void PaintAxis(vtkContext2D* painter, double size, int orientation);

  /**
   * Set up the axis title. Returns the bounding rect of all elements in the color legend.
   */
  void PaintTitle(vtkContext2D* painter, double size, const std::string& title, int orientation);

  /**
   * Compute the bounding box of the drawn axis.
   */
  vtkRectf ComputeAxisBoundingRect(double size, int orientation);

  /**
   * Used to render the 2D vtkContextScene.
   */
  vtkNew<vtkContextActor> ActorDelegate;

  /**
   * Justification of the titles (relative to the axis).
   */
  int TitleJustification = VTK_TEXT_LEFT;

  /**
   * Background Parameters.
   */
  double BackgroundColor[4] = { 1.0, 1.0, 1.0, 0.5 };
  double BackgroundPadding = 2.0;

  /**
   * Scalar bar size (square sides length).
   */
  double ScalarBarLength = 0.15;

  /**
   * Label parameters.
   */
  bool AutomaticLabelFormat = true;
  bool AddRangeLabels = true;
  std::string RangeLabelFormat = "%g";

  /**
   * Outline parameters.
   */
  bool DrawScalarBarOutline = true;
  double ScalarBarOutlineColor[3] = { 1.0, 1.0, 1.0 };
  int ScalarBarOutlineThickness = 1;

  /**
   * Flag to enable or disable drawing tick marks.
   */
  bool DrawTickMarks = true;

  /**
   * Texture being drawn in the scalar bar.
   */
  vtkSmartPointer<vtkTexture> Texture;

  /**
   * Data range of the horizontal and vertical axis.
   */
  double FirstRange[2] = { 0, 0 };
  double SecondRange[2] = { 0, 0 };

  /**
   * Title next to the vertical axis.
   */
  std::string Title2;

  /**
   * Charts API subclass we use to redirect the Paint() request back
   * to vtkContext2DTexturedScalarBarActor::Paint().
   */
  class vtkScalarBarItem;
  vtkNew<vtkScalarBarItem> ScalarBarItem;

  /**
   * Needed to convert normalized viewport coordinates to display coordinates.
   */
  vtkViewport* CurrentViewport = nullptr;

  /**
   * Axis for value labels, etc.
   */
  vtkNew<vtkAxis> Axis;

  /**
   * Keep track of whether we are currently computing the bounds.
   */
  bool InGetBoundingRect = false;

  /**
   * Stores the rect containing the full scalar bar actor.
   * This needs to be computed before painting to figure out how large a
   * background rectangle should be, but computing it during painting
   * leads to an infinite recursion, so we compute it in RenderOverlay()
   * before painting.
   */
  vtkRectf CurrentBoundingRect;
};

#endif // vtkContext2DTexturedScalarBarActor_h
