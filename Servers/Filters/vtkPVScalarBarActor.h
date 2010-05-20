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
// except that the fonts are set to a fixed size.  Adjustments to the scalar bar
// happen solely on the bar itself.  There are other slight differences in how
// the size of the scalar bar is determined to make it easier to control in
// ParaView.
//

#ifndef __vtkPVScalarBarActor_h
#define __vtkPVScalarBarActor_h

#include "vtkScalarBarActor.h"

#include "vtkSmartPointer.h" // For ivars
#include <vtkstd/vector>     // For ivars

class VTK_EXPORT vtkPVScalarBarActor : public vtkScalarBarActor
{
public:
  vtkTypeMacro(vtkPVScalarBarActor, vtkScalarBarActor);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPVScalarBarActor *New();

  // Description:
  // Draw the scalar bar and annotation text to the screen.
  virtual int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderOverlay(vtkViewport* viewport);

  // Description:
  // The bar aspect ratio (length/width).  Defaults to 20.  Note that this
  // the aspect ration of the color bar only, not including labels.
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
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkPVScalarBarActor();
  ~vtkPVScalarBarActor();

  double AspectRatio;

  int AutomaticLabelFormat;

//BTX
  // Description:
  // These replace the TextMappers and TextActors fields because they are a
  // lot easier to work with.
  vtkstd::vector<vtkSmartPointer<vtkTextMapper> > LabelMappers;
  vtkstd::vector<vtkSmartPointer<vtkActor2D> >    LabelActors;
//ETX

  // Description:
  // Allocate and position all the labels (stored in LabelMappers and
  // LabelActors).
  virtual void AllocateAndPositionLabels(int *propSize, vtkViewport *viewport);

  // Description:
  // Determines the size and position of the title and sets the mapper and
  // actor to place it there.
  virtual void PositionTitle(const int propSize[2], vtkViewport *viewport);

  // Description:
  // Set up the ScalarBar, ScalarBarMapper, and ScalarBarActor based on the
  // current position and orientation of this actor.
  virtual void PositionScalarBar(const int propSize[2], vtkViewport *viewport);

  // Description:
  // Set up the texture used to render the scalar bar.
  virtual void BuildScalarBarTexture();

  // Description:
  // A convenience function for creating one of the labels.  A text mapper
  // and associated actor are added to LabelMappers and LabelActors
  // respectively.  The index to the newly created entries is returned.
  virtual int CreateLabel(double value, int targetWidth, int targetHeight,
                          vtkViewport *viewport);

//BTX
  // Description:
  // Given a data range, finds locations for tick marks that will have
  // "friendly" labels (that is, can be represented with less units of
  // precision).
  virtual vtkstd::vector<double> LinearTickMarks(const double range[2],
                                                 int maxTicks,
                                                 bool intOnly=false);
  virtual vtkstd::vector<double> LogTickMarks(const double range[2],
                                              int maxTicks);
//ETX

  vtkTexture *ScalarBarTexture;

  vtkPolyData           *TickMarks;
  vtkPolyDataMapper2D   *TickMarksMapper;
  vtkActor2D            *TickMarksActor;

  // Description:
  // Space, in pixels, between the title and the rest of the bar.  Currently
  // set in PositionTitle.
  int TitleSpace;
  // Description:
  // The width and height of the bar in pixels.  Currently set in
  // AllocateAndPositionLabels.
  int BarWidth;
  int BarHeight;
  // Description:
  // Space, in pixels, between the labels and the bar itself.  Currently set in
  // PositionTitle.
  int LabelSpace;
  // Description:
  // The space in pixels given to the hieght of the labels.
  int LabelHeight;

private:
  vtkPVScalarBarActor(const vtkPVScalarBarActor &);     // Not implemented.
  void operator=(const vtkPVScalarBarActor &);          // Not implemented.
};

#endif //__vtkPVScalarBarActor_h
