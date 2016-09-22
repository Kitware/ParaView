/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBarActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkPVScalarBarActor - A scalar bar with labels of fixed font.
//
// .SECTION Description
//
// vtkPVScalarBarActor has basically the same functionality as vtkScalarBarActor
// except that the fonts are set to a fixed size and tick labels vary in precision
// before their size is adjusted to meet geometric constraints.
// These changes make it easier to control in ParaView.
//

#ifndef vtkPVScalarBarActor_h
#define vtkPVScalarBarActor_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkScalarBarActor.h"

#include "vtkNew.h" // For ivars
#include "vtkSmartPointer.h" // For ivars
#include <vector> // For ivars

class vtkAxis;
class vtkContextScene;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVScalarBarActor : public vtkScalarBarActor
{
public:
  vtkTypeMacro(vtkPVScalarBarActor, vtkScalarBarActor);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPVScalarBarActor *New();

  // Description:
  // The bar aspect ratio (length/width).  Defaults to 20.  Note that this
  // the aspect ratio of the color bar only, not including labels.
  vtkGetMacro(AspectRatio, double);
  vtkSetMacro(AspectRatio, double);

  // Description:
  // If true (the default), the printf format used for the labels will be
  // automatically generated to make the numbers best fit within the widget.  If
  // false, the LabelFormat ivar will be used.
  vtkGetMacro(AutomaticLabelFormat, int);
  vtkSetMacro(AutomaticLabelFormat, int);
  vtkBooleanMacro(AutomaticLabelFormat, int);

  // Description:
  // If true (the default), tick marks will be drawn.
  vtkGetMacro(DrawTickMarks, int);
  vtkSetMacro(DrawTickMarks, int);
  vtkBooleanMacro(DrawTickMarks, int);

  // Description:
  // If true (the default), sub-tick marks will be drawn.
  vtkGetMacro(DrawSubTickMarks, int);
  vtkSetMacro(DrawSubTickMarks, int);
  vtkBooleanMacro(DrawSubTickMarks, int);

  // Description:
  // Set whether the range endpoints (minimum and maximum) are added
  // as labels alongside other value labels.
  vtkGetMacro(AddRangeLabels, int);
  vtkSetMacro(AddRangeLabels, int);
  vtkBooleanMacro(AddRangeLabels, int);

  // Description:
  // Set whether annotions are automatically created according the number
  // of discrete colors. Default is FALSE;
  vtkSetMacro(AutomaticAnnotations, int);
  vtkGetMacro(AutomaticAnnotations, int);
  vtkBooleanMacro(AutomaticAnnotations, int);

  // Description:
  // Set the C-style format string for the range labels.
  vtkGetStringMacro(RangeLabelFormat);
  vtkSetStringMacro(RangeLabelFormat);

  // Description:
  // Add value as annotation label on scalar bar at the given position
  virtual void AddValueLabelIfUnoccluded(
    double value, double pos, double diff);

  // Description:
  // Set the title justification. Valid values are VTK_TEXT_LEFT,
  // VTK_TEXT_CENTERED, and VTK_TEXT_RIGHT.
  vtkGetMacro(TitleJustification, int);
  vtkSetClampMacro(TitleJustification, int, VTK_TEXT_LEFT, VTK_TEXT_RIGHT);

  // Description:
  // Set whether the scalar data range endpoints (minimum and maximum)
  // are added as annotations.
  vtkGetMacro(AddRangeAnnotations, int);
  vtkSetMacro(AddRangeAnnotations, int);
  vtkBooleanMacro(AddRangeAnnotations, int);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow*);

  // Description:
  // Overridden to sync internal variables with renderer state.
  int RenderOpaqueGeometry(vtkViewport* viewport);

  // Description:
  // Draw the scalar bar and annotation text to the screen.
  virtual int RenderOverlay(vtkViewport* viewport);

protected:
  vtkPVScalarBarActor();
  ~vtkPVScalarBarActor();

  // Description:
  // These methods override the subclass implementation.
  virtual void PrepareTitleText();
  virtual void ComputeScalarBarThickness();
  virtual void LayoutTitle();
  virtual void ComputeScalarBarLength();
  virtual void LayoutTicks();
  virtual void ConfigureAnnotations();
  virtual void ConfigureTitle();
  virtual void ConfigureTicks();

  // Description:
  // Annotate the min/max values on the scalar bar (in interval/ratio mode).
  //
  // This overrides the subclass implementation.
  virtual void EditAnnotations();

  // Description:
  // Set up the ScalarBar, ScalarBarMapper, and ScalarBarActor based on the
  // current position and orientation of this actor.
  //virtual void PositionScalarBar(const int propSize[2], vtkViewport *viewport);

  // Description:
  // Set up the texture used to render the scalar bar.
  virtual void BuildScalarBarTexture();

  // Description:
  // A convenience function for creating one of the labels.  A text mapper
  // and associated actor are added to LabelMappers and LabelActors
  // respectively.  The index to the newly created entries is returned.
  virtual int CreateLabel(
    double value, int minDigits,
    int targetWidth, int targetHeight, vtkViewport* viewport);

  double AspectRatio;
  int AutomaticLabelFormat;
  int DrawTickMarks;
  int DrawSubTickMarks;
  int AddRangeLabels;

  // Description:
  // Flag indicating whether automatic annotations are computed and shown.
  int AutomaticAnnotations;

  char* RangeLabelFormat;

  vtkTexture* ScalarBarTexture;
  vtkPolyData* TickMarks;
  vtkPolyDataMapper2D* TickMarksMapper;
  vtkActor2D* TickMarksActor;

  // Description:
  // These are used to calculate the tick spacing.
  vtkNew<vtkAxis> TickLayoutHelper;
  vtkNew<vtkContextScene> TickLayoutHelperScene;

  // Description:
  // Space, in pixels, between the labels and the bar itself.  Currently set in
  // PositionTitle.
  int LabelSpace;

  // Description:
  // The justification/alignment of the title.
  int TitleJustification;

  // Description:
  // Flag to add minimum and maximum as annotations
  int AddRangeAnnotations;

private:
  vtkPVScalarBarActor(const vtkPVScalarBarActor &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVScalarBarActor &) VTK_DELETE_FUNCTION;
};

#endif //vtkPVScalarBarActor_h
