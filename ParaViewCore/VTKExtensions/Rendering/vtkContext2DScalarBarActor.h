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

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
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
class vtkScalarBarItem;
class vtkTextProperty;
class vtkViewport;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkContext2DScalarBarActor : public vtkScalarBarActor
{
public:
  vtkTypeMacro(vtkContext2DScalarBarActor, vtkScalarBarActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkContext2DScalarBarActor* New();

  //@{
  /**
   * Control the orientation of the scalar bar.
   */
  vtkSetClampMacro(Orientation, int, VTK_ORIENT_HORIZONTAL, VTK_ORIENT_VERTICAL);
  vtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal() { this->SetOrientation(VTK_ORIENT_HORIZONTAL); }
  void SetOrientationToVertical() { this->SetOrientation(VTK_ORIENT_VERTICAL); }
  //@}

  //@{
  /**
   * Set/Get the title text property.
   */
  virtual void SetTitleTextProperty(vtkTextProperty* p) VTK_OVERRIDE;
  vtkGetObjectMacro(TitleTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the labels text property.
   */
  virtual void SetLabelTextProperty(vtkTextProperty* p) VTK_OVERRIDE;
  vtkGetObjectMacro(LabelTextProperty, vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the title of the scalar bar actor.
   */
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the title for the component that is selected.
   */
  vtkSetStringMacro(ComponentTitle);
  vtkGetStringMacro(ComponentTitle);
  //@}

  //@{
  /**
   * Set the title justification. Valid values are VTK_TEXT_LEFT,
   * VTK_TEXT_CENTERED, and VTK_TEXT_RIGHT.
   */
  vtkGetMacro(TitleJustification, int);
  vtkSetClampMacro(TitleJustification, int, VTK_TEXT_LEFT, VTK_TEXT_RIGHT);
  //@}

  enum
  {
    PrecedeScalarBar = 0,
    SucceedScalarBar
  };

  //@{
  /**
   * Should the title and tick marks precede the scalar bar or succeed it?
   * This is measured along the viewport coordinate direction perpendicular
   * to the long axis of the scalar bar, not the reading direction.
   * Thus, succeed implies the that the text is above scalar bar if
   * the orientation is horizontal or right of scalar bar if the orientation
   * is vertical. Precede is the opposite.
   */
  vtkSetClampMacro(TextPosition, int, PrecedeScalarBar, SucceedScalarBar);
  vtkGetMacro(TextPosition, int);
  virtual void SetTextPositionToPrecedeScalarBar() VTK_OVERRIDE
  {
    this->SetTextPosition(vtkContext2DScalarBarActor::PrecedeScalarBar);
  }
  virtual void SetTextPositionToSucceedScalarBar() VTK_OVERRIDE
  {
    this->SetTextPosition(vtkContext2DScalarBarActor::SucceedScalarBar);
  }
  //@}

  /**
   * Get the PositionCoordinate instance of vtkCoordinate.
   * This is used for complicated or relative positioning.
   * The position variable controls the lower left corner of the Actor2D.
   */
  vtkViewportCoordinateMacro(Position);

  /**
   * Access the Position2 instance variable. This variable controls
   * the upper right corner of the Actor2D. It is by default
   * relative to Position and in normalized viewport coordinates.
   * Some 2D actor subclasses ignore the position2 variable.
   */
  vtkViewportCoordinateMacro(Position2);

  //@{
  /**
   * Set the scalar bar thickness. When the orientation is
   * VTK_ORIENT_VERTICAL, this sets the scalar bar width.  When the
   * orientation is VTK_ORIENT_HORIZONTAL, this sets the scalar bar
   * height.
   */
  vtkSetMacro(ScalarBarThickness, int);
  vtkGetMacro(ScalarBarThickness, int);
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
  virtual int RenderOverlay(vtkViewport* viewport) VTK_OVERRIDE;

  /**
   * Draw the scalar bar and annotation text to the screen.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow* window) VTK_OVERRIDE;

  /**
   * Responsible for actually drawing the scalar bar.
   */
  virtual bool Paint(vtkContext2D* painter);

protected:
  vtkContext2DScalarBarActor();
  virtual ~vtkContext2DScalarBarActor();

private:
  vtkContext2DScalarBarActor(const vtkContext2DScalarBarActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContext2DScalarBarActor&) VTK_DELETE_FUNCTION;

  vtkContextActor* ActorDelegate;

  /**
   * Vertical or horizontal orientation.
   */
  int Orientation;

  /**
   * Location of window, such as a corner or middle side position.
   */
  int WindowLocation;

  /**
   * Title properties.
   */
  char* Title;
  char* ComponentTitle;
  int TitleJustification;

  /**
   * Designates on which side of the color bar the labels/annotations appear.
   */
  int TextPosition;

  /**
   * Thickenss of the color bar, specified in pixels.
   */
  int ScalarBarThickness;

  /**
   * Font for the legend title.
   */
  vtkTextProperty* TitleTextProperty;

  /**
   * Font for tick+annotation labels.
   */
  vtkTextProperty* LabelTextProperty;

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
   * Size of ticks in terms of display coordinates.
   */
  double TickSize;

  /**
   * Should ticks be drawn?
   */
  bool DrawTickMarks;

  bool UseCustomLabels;

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
   * coordinates.
   */
  void GetSize(double size[2]);

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
