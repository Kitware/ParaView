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
// .NAME vtkContext2DScalarBarActor - custom scalar bar actor for ParaView
// that uses the Charts API
//
// .SECTION Description
//
// vtkContext2DScalarBarActor is a custom scalar bar actor that uses the
// Charts API for drawing calls. As the scalar bar actor is inherently 2D,
// the Charts API offers a comparatively simpler way of implementing features
// in the scalar bar actor than using vtkPolyData primitives.
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
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  static vtkContext2DScalarBarActor* New();

  // Description:
  // Control the orientation of the scalar bar.
  vtkSetClampMacro(Orientation, int, VTK_ORIENT_HORIZONTAL, VTK_ORIENT_VERTICAL);
  vtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal() { this->SetOrientation(VTK_ORIENT_HORIZONTAL); }
  void SetOrientationToVertical() { this->SetOrientation(VTK_ORIENT_VERTICAL); }

  // Description:
  // Set/Get the title text property.
  virtual void SetTitleTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(TitleTextProperty, vtkTextProperty);

  // Description:
  // Set/Get the labels text property.
  virtual void SetLabelTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(LabelTextProperty, vtkTextProperty);

  // Description:
  // Set/Get the title of the scalar bar actor,
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

  // Description:
  // Set/Get the title for the component that is selected,
  vtkSetStringMacro(ComponentTitle);
  vtkGetStringMacro(ComponentTitle);

  // Description:
  // Set the title justification. Valid values are VTK_TEXT_LEFT,
  // VTK_TEXT_CENTERED, and VTK_TEXT_RIGHT.
  vtkGetMacro(TitleJustification, int);
  vtkSetClampMacro(TitleJustification, int, VTK_TEXT_LEFT, VTK_TEXT_RIGHT);

  enum
  {
    PrecedeScalarBar = 0,
    SucceedScalarBar
  };

  // Description:
  // Should the title and tick marks precede the scalar bar or succeed it?
  // This is measured along the viewport coordinate direction perpendicular
  // to the long axis of the scalar bar, not the reading direction.
  // Thus, succeed implies the that the text is above scalar bar if
  // the orientation is horizontal or right of scalar bar if the orientation
  // is vertical. Precede is the opposite.
  vtkSetClampMacro(TextPosition, int, PrecedeScalarBar, SucceedScalarBar);
  vtkGetMacro(TextPosition, int);
  virtual void SetTextPositionToPrecedeScalarBar()
  {
    this->SetTextPosition(vtkContext2DScalarBarActor::PrecedeScalarBar);
  }
  virtual void SetTextPositionToSucceedScalarBar()
  {
    this->SetTextPosition(vtkContext2DScalarBarActor::SucceedScalarBar);
  }

  // Description:
  // Get the PositionCoordinate instance of vtkCoordinate.
  // This is used for complicated or relative positioning.
  // The position variable controls the lower left corner of the Actor2D
  vtkViewportCoordinateMacro(Position);

  // Description:
  // Access the Position2 instance variable. This variable controls
  // the upper right corner of the Actor2D. It is by default
  // relative to Position and in normalized viewport coordinates.
  // Some 2D actor subclasses ignore the position2 variable
  vtkViewportCoordinateMacro(Position2);

  // Description:
  // Set the scalar bar thickness. When the orientation is
  // VTK_ORIENT_VERTICAL, this sets the scalar bar width.  When the
  // orientation is VTK_ORIENT_HORIZONTAL, this sets the scalar bar
  // height.
  vtkSetMacro(ScalarBarThickness, int);
  vtkGetMacro(ScalarBarThickness, int);

  // Description:
  // If true (the default), the printf format used for the labels will be
  // automatically generated to make the numbers best fit within the widget.  If
  // false, the LabelFormat ivar will be used.
  vtkGetMacro(AutomaticLabelFormat, int);
  vtkSetMacro(AutomaticLabelFormat, int);
  vtkBooleanMacro(AutomaticLabelFormat, int);

  // Description:
  // Set/get whether to add range labels or not. These are labels that have
  // the minimum/maximum values of the scalar bar range.
  vtkSetMacro(AddRangeLabels, int);
  vtkGetMacro(AddRangeLabels, int);

  // Description:
  // Set whether annotions are automatically created according the number
  // of discrete colors. Default is FALSE;
  vtkSetMacro(AutomaticAnnotations, int);
  vtkGetMacro(AutomaticAnnotations, int);
  vtkBooleanMacro(AutomaticAnnotations, int);

  // Description:
  // Set whether the scalar data range endpoints (minimum and maximum)
  // are added as annotations.
  vtkGetMacro(AddRangeAnnotations, int);
  vtkSetMacro(AddRangeAnnotations, int);
  vtkBooleanMacro(AddRangeAnnotations, int);

  // Description:
  // Set/get whether tick marks should be drawn.
  vtkSetMacro(DrawTickMarks, bool);
  vtkGetMacro(DrawTickMarks, bool);

  // Description:
  // Printf format for range labels.
  vtkSetStringMacro(RangeLabelFormat);
  vtkGetStringMacro(RangeLabelFormat);

  // Description:
  // Use custom labels.
  void SetUseCustomLabels(bool useLabels);
  vtkGetMacro(UseCustomLabels, bool);

  // Description:
  // Set number of custom labels.
  void SetNumberOfCustomLabels(vtkIdType numLabels);
  vtkIdType GetNumberOfCustomLabels();

  // Description:
  // Set label for index.
  void SetCustomLabel(vtkIdType index, double value);

  // Description:
  // We only render in the overlay for the context scene.
  virtual int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Draw the scalar bar and annotation text to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow* window);

  // Description:
  // Responsible for actually drawing the scalar bar.
  virtual bool Paint(vtkContext2D* painter);

protected:
  vtkContext2DScalarBarActor();
  virtual ~vtkContext2DScalarBarActor();

private:
  vtkContext2DScalarBarActor(const vtkContext2DScalarBarActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContext2DScalarBarActor&) VTK_DELETE_FUNCTION;

  vtkContextActor* ActorDelegate;

  // Vertical or horizontal orientation
  int Orientation;

  // Location of window, such as a corner or middle side position
  int WindowLocation;

  // Title properties
  char* Title;
  char* ComponentTitle;
  int TitleJustification;

  // Designates on which side of the color bar the labels/annotations
  // appear.
  int TextPosition;

  // Thickenss of the color bar, specified in pixels
  int ScalarBarThickness;

  // Text properties
  vtkTextProperty* TitleTextProperty; //!< Font for the legend title.
  vtkTextProperty* LabelTextProperty; //!< Font for tick+annotation labels.

  int AutomaticLabelFormat;

  int AddRangeLabels;
  int AutomaticAnnotations;
  int AddRangeAnnotations;
  char* RangeLabelFormat;

  // Flag that controls whether an outline is drawn around the scalar bar.
  int OutlineScalarBar;

  // Spacer between color swatches in the long dimension of the scalar
  // bar. Defined in terms of display coordinates.
  double Spacer;

  // Size of ticks in terms of display coordinates.
  double TickSize;

  // Should ticks be drawn?
  bool DrawTickMarks;

  bool UseCustomLabels;

  // Custom label values
  vtkSmartPointer<vtkDoubleArray> CustomLabels;

  // Charts API subclass we use to redirect the Paint() request back
  // to vtkContext2DScalarBarActor::Paint()
  class vtkScalarBarItem;
  vtkScalarBarItem* ScalarBarItem;

  // Needed to convert Position{2}Coordinate to display coordinates.
  vtkViewport* CurrentViewport;

  // Axis for value labels, etc.
  vtkAxis* Axis;

  // Description:
  // Update the image data used to display the colors in a continuous
  // color map.
  void UpdateScalarBarTexture(vtkImageData* image);

  // Description:
  // Get the size of the color bar area as determined by the position coordinates
  void GetSize(double size[2]);

  // Description:
  // Compute the rect where the color bar will be displayed. This is
  // the rect containing only the color map, not the out-of-range
  // or NaN color swatches
  vtkRectf GetColorBarRect(double size[2]);

  // Description:
  // Compute the rect that contains the out-of-range color swatches
  // and the color map, but not the NaN color swatch.
  vtkRectf GetFullColorBarRect(double size[2]);

  // Description:
  // Get the rect for the above-range color swatch.
  vtkRectf GetAboveRangeColorRect(double size[2]);

  // Description:
  // Get the rect for the below-range color swatch.
  vtkRectf GetBelowRangeColorRect(double size[2]);

  // Description:
  // Get the rect for the NaN color swatch.
  vtkRectf GetNaNColorRect(double size[2]);

  // Description:
  // Copy appropriate text properties to the axis text properties
  void UpdateTextProperties();

  // Description:
  // Paint the color bar itself
  void PaintColorBar(vtkContext2D* painter, double size[2]);

  // Description:
  // Paint the axis and label positions
  void PaintAxis(vtkContext2D* painter, double size[2]);

  // Description:
  // Set up the axis title
  void PaintTitle(vtkContext2D* painter, double size[2]);

  class vtkAnnotationMap;

  // Description:
  // Draw annotations. These are notes anchored to values in
  // continuous color maps and the center of color swatches for
  // indexed color maps.
  void PaintAnnotations(vtkContext2D* painter, double size[2], const vtkAnnotationMap& map);

  void PaintAnnotationsVertically(
    vtkContext2D* painter, double size[2], const vtkAnnotationMap& map);

  void PaintAnnotationsHorizontally(
    vtkContext2D* painter, double size[2], const vtkAnnotationMap& map);
};

#endif // vtkContext2DScalarBarActor_h
