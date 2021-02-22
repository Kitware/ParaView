/*=========================================================================

  Program:   ParaView
  Module:    vtkContext2DScalarBarActor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** @class   vtkContext2DScalarBarActor
 *  @brief   Custom scalar bar actor for ParaView that uses the Charts API.
 *
 * .SECTION Description
 *
 * vtkContext2DScalarBarActor is a custom scalar bar actor that uses the
 * Charts API for drawing calls. As the scalar bar actor is inherently 2D,
 * the Charts API offers a comparatively simpler way of implementing features
 * in the scalar bar actor than using vtkPolyData primitives.
 */

#ifndef vtkContext2DScalarBarActor_h
#define vtkContext2DScalarBarActor_h

#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkScalarBarActor.h"

#include "vtkCoordinate.h"   // for Position and Position 2
#include "vtkRect.h"         // for functions that return vtkRects
#include "vtkSmartPointer.h" // for smart pointers

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

class VTKREMOTINGVIEWS_EXPORT vtkContext2DScalarBarActor : public vtkScalarBarActor
{
public:
  vtkTypeMacro(vtkContext2DScalarBarActor, vtkScalarBarActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkContext2DScalarBarActor* New();

  //@{
  /**
   * Set the title justification. Valid values are VTK_TEXT_LEFT,
   * VTK_TEXT_CENTERED, and VTK_TEXT_RIGHT.
   */
  vtkGetMacro(TitleJustification, int);
  vtkSetClampMacro(TitleJustification, int, VTK_TEXT_LEFT, VTK_TEXT_RIGHT);
  //@}

  //@{
  /**
   * If this flag is on, the title will be drawn horizontally above the
   * scalar bar. Off by default.
   */
  vtkSetMacro(ForceHorizontalTitle, bool);
  vtkGetMacro(ForceHorizontalTitle, bool);
  vtkBooleanMacro(ForceHorizontalTitle, bool);
  //@}

  enum
  {
    PrecedeScalarBar = 0,
    SucceedScalarBar
  };

  enum OutOfRangeType
  {
    BELOW_RANGE = 0,
    ABOVE_RANGE = 1,
  };

  //@{
  /**
   * Set the scalar bar thickness. When the orientation is
   * VTK_ORIENT_VERTICAL, this sets the scalar bar width.  When the
   * orientation is VTK_ORIENT_HORIZONTAL, this sets the scalar bar
   * height. Specified in points akin to font size.
   */
  vtkSetClampMacro(ScalarBarThickness, int, 0, VTK_INT_MAX);
  vtkGetMacro(ScalarBarThickness, int);
  //@}

  //@{
  /**
   * Set the scalar bar length. When the orientation is VTK_ORIENT_VERTICAL,
   * this sets the scalar bar height. When the orientation is
   * VTK_ORIENT_HORIZONTAL, this sets the scalar bar width. Specified in
   * normalized viewport coordinates, meaning the value is the fractional span
   * of the viewport's width or height in the range [0, 1], depending on
   * orientation.
   */
  vtkSetClampMacro(ScalarBarLength, double, 0, 1);
  vtkGetMacro(ScalarBarLength, double);
  //@}

  //@{
  /**
   * Set whether the scalar bar is reversed ie from high
   * to low instead of from low to high. Default is false;
   */
  vtkSetMacro(ReverseLegend, bool);
  vtkGetMacro(ReverseLegend, bool);
  vtkBooleanMacro(ReverseLegend, bool);
  //@}

  //@{
  /**
   * If true (the default), the printf format used for the labels will be
   * automatically generated to make the numbers best fit within the widget.  If
   * false, the LabelFormat ivar will be used.
   */
  vtkGetMacro(AutomaticLabelFormat, int);
  vtkSetMacro(AutomaticLabelFormat, int);
  vtkBooleanMacro(AutomaticLabelFormat, int);
  //@}

  //@{
  /**
   * Set/get whether to add range labels or not. These are labels that have
   * the minimum/maximum values of the scalar bar range.
   */
  vtkSetMacro(AddRangeLabels, int);
  vtkGetMacro(AddRangeLabels, int);
  //@}

  //@{
  /**
   * Set whether annotions are automatically created according the number
   * of discrete colors. Default is FALSE;
   */
  vtkSetMacro(AutomaticAnnotations, int);
  vtkGetMacro(AutomaticAnnotations, int);
  vtkBooleanMacro(AutomaticAnnotations, int);
  //@}

  //@{
  /**
   * Set whether the scalar data range endpoints (minimum and maximum)
   * are added as annotations.
   */
  vtkGetMacro(AddRangeAnnotations, int);
  vtkSetMacro(AddRangeAnnotations, int);
  vtkBooleanMacro(AddRangeAnnotations, int);
  //@}

  //@{
  /**
   * Set/get whether tick marks should be drawn.
   */
  vtkSetMacro(DrawTickMarks, bool);
  vtkGetMacro(DrawTickMarks, bool);
  //@}

  //@{
  /**
   * Printf format for range labels.
   */
  vtkSetStringMacro(RangeLabelFormat);
  vtkGetStringMacro(RangeLabelFormat);
  //@}

  //@{
  /**
   * Use custom labels.
   */
  void SetUseCustomLabels(bool useLabels);
  vtkGetMacro(UseCustomLabels, bool);
  //@}

  //@{
  /**
   * Set number of custom labels.
   */
  void SetNumberOfCustomLabels(vtkIdType numLabels);
  vtkIdType GetNumberOfCustomLabels();
  //@}

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

  /**
   * Get an estimated number of annotations emulating loosely the
   * algorithm generating the annotations. The actual number of annotations
   * can be slightly lower than the return of this method when using
   * automatic annotations.
   */
  int GetEstimatedNumberOfAnnotations();

protected:
  vtkContext2DScalarBarActor();
  ~vtkContext2DScalarBarActor() override;

private:
  vtkContext2DScalarBarActor(const vtkContext2DScalarBarActor&) = delete;
  void operator=(const vtkContext2DScalarBarActor&) = delete;

  vtkContextActor* ActorDelegate;

  int TitleJustification;

  /**
   * Force the title orientation to horizontal when the scalar bar is in
   * vertical orientation.
   */
  bool ForceHorizontalTitle;

  /**
   * Thickness of the color bar.
   */
  int ScalarBarThickness;

  /**
  * Length of the color bar.
  */
  double ScalarBarLength;

  int AutomaticLabelFormat;

  int AddRangeLabels;
  int AutomaticAnnotations;
  int AddRangeAnnotations;
  char* RangeLabelFormat;

  /**
   * Flag that controls whether an outline is drawn around the scalar bar.
   */
  int OutlineScalarBar;

  /**
   * Spacer between color swatches in the long dimension of the scalar
   * bar. Defined in terms of display coordinates.
   */
  double Spacer;

  /**
   * Should ticks be drawn?
   */
  bool DrawTickMarks;

  bool UseCustomLabels;

  /**
   * Flag that controls if the whole color legend should be reversed
   */
  bool ReverseLegend;

  /**
   * Custom label values.
   */
  vtkSmartPointer<vtkDoubleArray> CustomLabels;

  /**
   * Charts API subclass we use to redirect the Paint() request back
   * to vtkContext2DScalarBarActor::Paint().
   */
  class vtkScalarBarItem;
  vtkScalarBarItem* ScalarBarItem;

  /**
   * Needed to convert Position{2}Coordinate to display coordinates.
   */
  vtkViewport* CurrentViewport;

  /**
   * Axis for value labels, etc.
   */
  vtkAxis* Axis;

  /**
   * Update the image data used to display the colors in a continuous
   * color map.
   */
  void UpdateScalarBarTexture(vtkImageData* image);

  /**
   * Get the size of the color bar area as determined by the position
   * coordinates. The painter parameter is needed to calculate the thickness
   * of the scalar bar in terms of the selected font size.
   */
  void GetSize(double size[2], vtkContext2D* painter);

  /**
   * Compute the rect where the color bar will be displayed. This is
   * the rect containing only the color map, not the out-of-range
   * or NaN color swatches.
   */
  vtkRectf GetColorBarRect(double size[2]);

  /**
   * Compute the rect that contains the out-of-range color swatches
   * and the color map, but not the NaN color swatch.
   */
  vtkRectf GetFullColorBarRect(double size[2]);

  /**
   * Get the rect for the above-range color swatch.
   */
  vtkRectf GetAboveRangeColorRect(double size[2]);

  /**
   * Get the rect for the below-range color swatch.
   */
  vtkRectf GetBelowRangeColorRect(double size[2]);

  vtkRectf GetOutOfRangeColorRectInternal(
    vtkContext2DScalarBarActor::OutOfRangeType type, double size[2]);

  /**
   * Get the rect for the NaN color swatch.
   */
  vtkRectf GetNaNColorRect(double size[2]);

  /**
   * Copy appropriate text properties to the axis text properties.
   */
  void UpdateTextProperties();

  /**
   * Paint the color bar itself.
   */
  void PaintColorBar(vtkContext2D* painter, double size[2]);

  /**
   * Paint the axis and label positions.
   */
  void PaintAxis(vtkContext2D* painter, double size[2]);

  /**
   * Set up the axis title.
   */
  void PaintTitle(vtkContext2D* painter, double size[2]);

  class vtkAnnotationMap;

  /**
   * Draw annotations. These are notes anchored to values in
   * continuous color maps and the center of color swatches for
   * indexed color maps.
   */
  void PaintAnnotations(vtkContext2D* painter, double size[2], const vtkAnnotationMap& map);

  /**
   * Responsible for painting the annoations vertically.
   */
  void PaintAnnotationsVertically(
    vtkContext2D* painter, double size[2], const vtkAnnotationMap& map);

  /**
   * Responsible for painting the annotations horizontally.
   */
  void PaintAnnotationsHorizontally(
    vtkContext2D* painter, double size[2], const vtkAnnotationMap& map);
};

#endif // vtkContext2DScalarBarActor_h
