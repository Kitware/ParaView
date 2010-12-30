/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBarActor.cxx

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

#include "vtkPVScalarBarActor.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTextActor.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkWindow.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

#include <vtkstd/algorithm>

#define COLOR_TEXTURE_MAP_SIZE 256

#define MY_ABS(x)       ((x) < 0 ? -(x) : (x))

//=============================================================================
vtkStandardNewMacro(vtkPVScalarBarActor);

//=============================================================================
vtkPVScalarBarActor::vtkPVScalarBarActor()
{
  this->AspectRatio = 20.0;
  this->AutomaticLabelFormat = 1;

  this->ScalarBarTexture = vtkTexture::New();

  this->TickMarks = vtkPolyData::New();
  this->TickMarksMapper = vtkPolyDataMapper2D::New();
  this->TickMarksMapper->SetInput(this->TickMarks);
  this->TickMarksActor = vtkActor2D::New();
  this->TickMarksActor->SetMapper(this->TickMarksMapper);
  this->TickMarksActor->GetPositionCoordinate()
    ->SetReferenceCoordinate(this->PositionCoordinate);
}

vtkPVScalarBarActor::~vtkPVScalarBarActor()
{
  this->ScalarBarTexture->Delete();

  this->TickMarks->Delete();
  this->TickMarksMapper->Delete();
  this->TickMarksActor->Delete();

  this->LabelMappers.clear();
  this->LabelActors.clear();

  if (this->ComponentTitle)
    {
    delete [] this->ComponentTitle;
    this->ComponentTitle = NULL;
    }
}

void vtkPVScalarBarActor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent); 
  os << indent << "AspectRatio: " << this->AspectRatio << endl;
  os << indent << "AutomaticLabelFormat: " << this->AutomaticLabelFormat <<endl;
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::ReleaseGraphicsResources(vtkWindow *window)
{
  this->ScalarBarTexture->ReleaseGraphicsResources(window);

  for (unsigned int i = 0; i < this->LabelActors.size(); i++)
    {
    this->LabelActors[i]->ReleaseGraphicsResources(window);
    }

  this->TickMarksActor->ReleaseGraphicsResources(window);

  this->Superclass::ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
int vtkPVScalarBarActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;
  
  if (this->UseOpacity)
    {
    this->Texture->Render(vtkRenderer::SafeDownCast(viewport));
    renderedSomething += this->TextureActor->RenderOverlay(viewport);
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }
  this->ScalarBarTexture->Render(vtkRenderer::SafeDownCast(viewport));
  renderedSomething += this->ScalarBarActor->RenderOverlay(viewport);

  renderedSomething += this->TickMarksActor->RenderOverlay(viewport);

  for (unsigned int i = 0; i < this->LabelActors.size(); i++)
    {
    renderedSomething += this->LabelActors[i]->RenderOverlay(viewport);
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

//----------------------------------------------------------------------------
int vtkPVScalarBarActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;
  int size[2];
  
  if (!this->LookupTable)
    {
    vtkWarningMacro(<<"Need a lookup table to render a scalar bar");
    return 0;
    }

  if (!this->TitleTextProperty)
    {
    vtkErrorMacro(<<"Need title text property to render a scalar bar");
    return 0;
    }

  if (!this->LabelTextProperty)
    {
    vtkErrorMacro(<<"Need label text property to render a scalar bar");
    return 0;
    }

  // Check to see whether we have to rebuild everything
  int positionsHaveChanged = 0;
  if (viewport->GetMTime() > this->BuildTime || 
      (viewport->GetVTKWindow() && 
       viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
    {
    // if the viewport has changed we may - or may not need
    // to rebuild, it depends on if the projected coords chage
    int *barOrigin;
    barOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      barOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      barOrigin[1];
    if (this->LastSize[0] != size[0] || 
        this->LastSize[1] != size[1] ||
        this->LastOrigin[0] != barOrigin[0] || 
        this->LastOrigin[1] != barOrigin[1])
      {
      positionsHaveChanged = 1;
      }
    }
  
  // Check to see whether we have to rebuild everything
  if (positionsHaveChanged ||
      this->GetMTime() > this->BuildTime || 
      this->LookupTable->GetMTime() > this->BuildTime ||
      this->LabelTextProperty->GetMTime() > this->BuildTime ||
      this->TitleTextProperty->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding subobjects");

    // Delete previously constructed objects
    this->LabelMappers.clear();
    this->LabelActors.clear();
    this->ScalarBarActor->GetProperty()->DeepCopy(this->GetProperty());
    this->TickMarksActor->GetProperty()->DeepCopy(this->GetProperty());

    // get the viewport size in display coordinates
    int *barOrigin;
    barOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      barOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      barOrigin[1];
    this->LastOrigin[0] = barOrigin[0];
    this->LastOrigin[1] = barOrigin[1];
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
    
    // Update all the composing objects
    this->TitleActor->GetProperty()->DeepCopy(this->GetProperty());
    if ( this->ComponentTitle && strlen(this->ComponentTitle) > 0 )
      {
      //need to account for a space between title & component and null term
      int str_len = strlen(this->Title) + strlen(this->ComponentTitle) + 2;
      char *combinedTitle = new char[ str_len ];
      strcpy(combinedTitle, this->Title );
      strcat( combinedTitle, " " );
      strcat( combinedTitle, this->ComponentTitle );
      this->TitleMapper->SetInput(combinedTitle);
      delete [] combinedTitle;
      }
    else
      {
      this->TitleMapper->SetInput(this->Title);
      }
    // find the best size for the title font
    this->PositionTitle(size, viewport);
    
    // find the best size for the ticks
    this->AllocateAndPositionLabels(size, viewport);

    this->PositionScalarBar(size, viewport);

    // TODO: Only call this when the colors change.
    this->BuildScalarBarTexture();

    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }
  this->ScalarBarTexture->Render(vtkRenderer::SafeDownCast(viewport));
  renderedSomething += this->ScalarBarActor->RenderOpaqueGeometry(viewport);
  renderedSomething += this->TickMarksActor->RenderOpaqueGeometry(viewport);
  for (unsigned int i = 0; i < this->LabelActors.size(); i++)
    {
    renderedSomething += this->LabelActors[i]->RenderOverlay(viewport);
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkPVScalarBarActor::CreateLabel(double value,
                                     int targetWidth, int targetHeight,
                                     vtkViewport *viewport)
{
  char string[1024];

  VTK_CREATE(vtkTextMapper, textMapper);

  // Shallow copy here so that the size of the label prop is not affected by the
  // automatic adjustment of its text mapper's size (i.e. its mapper's text
  // property is identical except for the font size which will be modified
  // later). This allows text actors to share the same text property, and in
  // that case specifically allows the title and label text prop to be the same.
  textMapper->GetTextProperty()->ShallowCopy(this->LabelTextProperty);

  if (this->AutomaticLabelFormat)
    {
    // Iterate over all format lengths and find the highest precision that we
    // can represent without going over the target width.  If we cannot fit
    // within the target width, make the smallest possible text.
    int smallestFoundWidth = VTK_INT_MAX;
    bool foundValid = false;
    string[0] = '\0';
    for (int i = 1; i < 20; i++)
      {
      char format[512];
      char string2[1024];
      sprintf(format, "%%-0.%dg", i);
      sprintf(string2, format, value);

      //we want the reduced size used so that we can get better fitting
      // Extra filter: Used to remove unwanted 0 after e+ or e-
      // i.e.: 1.23e+009 => 1.23e+9
      vtkstd::string strToFilter = string2;
      vtkstd::string ePlus = "e+0";
      vtkstd::string eMinus = "e-0";
      size_t pos = 0;
      while( (pos = strToFilter.find(ePlus)) != vtkstd::string::npos ||
             (pos = strToFilter.find(eMinus)) != vtkstd::string::npos)
        {
        strToFilter.erase(pos + 2, 1);
        }
      strcpy(string2, strToFilter.c_str());
      textMapper->SetInput(string2);

      textMapper->SetConstrainedFontSize(viewport, VTK_INT_MAX, targetHeight);
      int actualWidth = textMapper->GetWidth(viewport);
      if (actualWidth < targetWidth)
        {
        // Found a string that fits.  Keep it unless we find something better.
        strcpy(string, string2);
        foundValid = true;
        }
      else if ((actualWidth < smallestFoundWidth) && !foundValid)
        {
        // String does not fit, but it is the smallest so far.
        strcpy(string, string2);
        smallestFoundWidth = actualWidth;
        }
      }
    }
  else
    {
    // Potential of buffer overrun (onto the stack) here.
    sprintf(string, this->LabelFormat, value);
    }

  // Set the txt label
  //cout << "Value: " << value << " converted to " << string << endl;
  textMapper->SetInput(string);

  // Size the font to fit in the targetHeight, which we are using
  // to size the font because it is (relatively?) constant.
  textMapper->SetConstrainedFontSize(viewport, VTK_INT_MAX, targetHeight);

  // Make sure that the string fits in the allotted space.
  if (textMapper->GetWidth(viewport) > targetWidth)
    {
    textMapper->SetConstrainedFontSize(viewport, targetWidth, targetHeight);
    }

  VTK_CREATE(vtkActor2D, textActor);
  textActor->SetMapper(textMapper);
  textActor->GetProperty()->DeepCopy(this->GetProperty());
  textActor->GetPositionCoordinate()->
                               SetReferenceCoordinate(this->PositionCoordinate);

  this->LabelMappers.push_back(textMapper);
  this->LabelActors.push_back(textActor);

  return static_cast<int>(this->LabelActors.size()) - 1;
}

//-----------------------------------------------------------------------------
vtkstd::vector<double> vtkPVScalarBarActor::LinearTickMarks(
                                                          const double range[2],
                                                          int maxTicks,
                                                          bool intOnly /*=0*/)
{
  vtkstd::vector<double> ticks;

  // Compute the difference between min and max of scalar bar values.
  double delta = range[1] - range[0];
  if (delta == 0) return ticks;

  // See what digit of the decimal number the difference is contained in.
  double dmag = log10(delta);
  double emag = floor(dmag) - 1;

  // Compute a preliminary "step size" for tic marks.
  double originalMag = pow(10.0, emag);
  // if ((originalMag > MY_ABS(range[0])) || (originalMag > MY_ABS(range[1])))
  if (1.1*originalMag > delta)
    {
    originalMag /= 10.0;
    }

  // Make sure we comply with intOnly request.
  if (intOnly)
    {
    originalMag = floor(originalMag);
    if (originalMag < 1.0) originalMag = 1.0;
    }

  // If we have too many ticks, try reducing the number of ticks by applying
  // these scaling factors to originalMag in this order.
  const double magScale[] = { 1.0, 2.0, 2.5, 4.0, 10.0,
                              20.0, 25.0, 40.0, 100.0 };
  const int numScales = static_cast<int>(sizeof(magScale)/sizeof(double));

  for (int scaleIdx = 0; scaleIdx < numScales; scaleIdx++)
    {
    double scale = magScale[scaleIdx];

    if (intOnly && scale == 2.5) continue;

    double mag = scale*originalMag;

    // Use this to get around some rounding errors.
    double tolerance = 0.0001*mag;

    // Round to a sensible number of digits.
    // Round minima towards the origin, maxima away from it.
    double mintrunc, maxtrunc;
    if (range[0] > 0)
      {
      mintrunc = floor(range[0]/mag) * mag;
      }
    else
      {
      mintrunc = ceil(range[0]/mag) * mag;
      }
    if (range[1] > 0)
      {
      maxtrunc = ceil(range[1]/mag) * mag;
      }
    else
      {
      maxtrunc = floor(range[1]/mag) * mag;
      }

    // Handle cases where rounding extends range.  (Note swapping floor/ceil
    // above doesn't work well because not all decimal numbers get represented
    // exactly in binary...) better to do this.
    if (mintrunc < range[0] - tolerance) mintrunc += mag;
    if (maxtrunc > range[1] + tolerance) maxtrunc -= mag;

#if 0
    // Figure out how many digits we must show in order for tic labels
    // to have at least one unique digit... this may get altered if we change
    // mag below.  (Note, that we are not using this at the moment since we show
    // as many digits as possible, but perhaps in the future we want it.)
    double nsd1 = ceil(log10(MY_ABS(mintrunc)/mag));
    double nsd2 = ceil(log10(MY_ABS(maxtrunc)/mag));
    int numSignificantDigits = vtkstd::max(nsd1, nsd2);
#endif

    // Compute the ticks.
    double tick;
    ticks.clear();
    for (int factor = 0; (tick = mintrunc+factor*mag) <= maxtrunc+tolerance;
         factor++)
      {
      ticks.push_back(tick);
      }
    int nticks = static_cast<int>(ticks.size());

    // If we have not exceeded limit, then we are done.
    if ((maxTicks <= 0) || (nticks <= maxTicks))
      {
      return ticks;
      }
    }

  // Can't seem to find good ticks.  Return nothing
  ticks.clear();
  return ticks;
}

//-----------------------------------------------------------------------------
vtkstd::vector<double> vtkPVScalarBarActor::LogTickMarks(const double range[2],
                                                         int maxTicks)
{
  vtkstd::vector<double> ticks;

  if (range[0] * range[1] <= 0)
    {
    vtkErrorMacro(<< "Can't have a plot that uses/crosses 0!" << endl
                  << "Freak OUT, man!");
    return ticks;
    }

  double logrange[2];
  logrange[0] = log10(range[0]);  logrange[1] = log10(range[1]);
  ticks = this->LinearTickMarks(logrange, maxTicks, true);

#if 0
  // Figure out how many digits we must show in order for tic labels
  // to have at least one unique digit... this may get altered if we change
  // mag below.  (Note, that we are not using this at the moment since we show
  // as many digits as possible, but perhaps in the future we want it.)
  if (ticks.length() > 1)
    {
    double ticksZ = pow(10, ticks[1] - ticks[0]);
    double nsd1 = range[0]/ticksZ;
    double nsd2 = range[1]/ticksZ;
    double numSignificantDigits = vtkstd::max(nsd1, nsd2);
    }
#endif

  for (size_t i = 0; i < ticks.size(); i++)
    {
    ticks[i] = pow(10.0, ticks[i]);
    }

  return ticks;
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::PositionTitle(const int propSize[2],
                                        vtkViewport *viewport)
{
  if (this->Title == NULL || !strlen(this->Title))
    {
    return;
    }

  // Reset the text size.
  this->TitleMapper->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
  this->TitleMapper->GetTextProperty()->SetJustificationToCentered();

  // Get the size of the font in pixels.
  int targetSize[2];
  this->TitleMapper->GetSize(viewport, targetSize);

  // Resize the font based on the viewport size.
  double fontScale = vtkTextActor::GetFontScale(viewport);
  targetSize[0] = (int)(fontScale*targetSize[0]);
  targetSize[1] = (int)(fontScale*targetSize[1]);

  // Ask the mapper to rescale the font based on the new metrics.
  this->TitleMapper->SetConstrainedFontSize(viewport,
                                            targetSize[0], targetSize[1]);

  // Get the actual size of the text.
  int titleSize[2];
  this->TitleMapper->GetSize(viewport, titleSize);

  // Position the title.
  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    this->TitleActor->SetPosition(propSize[0]/2, propSize[1]-titleSize[1]);
    }
  else // this->Orientation == VTK_ORIENT_HORIZONTAL
    {
    this->TitleActor->SetPosition(propSize[0]/2, propSize[1]-titleSize[1]);
    }

  this->TitleSpace = 4;
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::AllocateAndPositionLabels(int *propSize,
                                                    vtkViewport *viewport)
{
  // Get the size of the title.
  int titleSize[2];
  this->TitleMapper->GetSize(viewport, titleSize);

  double fontScaling = vtkTextActor::GetFontScale(viewport);

  // Create a dummy text mapper.  We are going to use it to figure out font
  // sizes.
  VTK_CREATE(vtkTextMapper, dummyMapper);
  dummyMapper->GetTextProperty()->ShallowCopy(this->LabelTextProperty);

  int targetHeight, targetWidth;

  // Initialize the height with the requested hight from the font (which
  // should be about the same for all mappers).
  dummyMapper->SetInput("()");
  targetHeight = (int)(fontScaling*dummyMapper->GetHeight(viewport));
  dummyMapper->SetConstrainedFontSize(viewport, VTK_INT_MAX, targetHeight);
  this->LabelHeight = dummyMapper->GetHeight(viewport);

  // Determine the bar dimensions.  Position will come later in
  // PositionScalarBar.
  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    this->BarHeight
      = propSize[1] - this->TitleSpace - titleSize[1] - 2*targetHeight;
    this->BarWidth = static_cast<int>(this->BarHeight/this->AspectRatio);
    this->LabelSpace = this->BarWidth/2 + 1;

    targetWidth = propSize[0];
    }
  else
    {
    this->BarWidth = propSize[0];
    this->BarHeight = static_cast<int>(this->BarWidth/this->AspectRatio);
    this->LabelSpace = this->BarHeight/2 + 1;

    targetWidth = propSize[0]/4;
    }

  // is this a vtkLookupTable or a subclass of vtkLookupTable 
  // with its scale set to log
  int isLogTable = this->LookupTable->UsingLogScale();

  double *range = this->LookupTable->GetRange();

  int labelIdx;
  labelIdx = this->CreateLabel(range[0], targetWidth, targetHeight, viewport);
  vtkTextMapper *minTextMapper = this->LabelMappers[labelIdx];
  vtkActor2D *minTextActor = this->LabelActors[labelIdx];
  labelIdx = this->CreateLabel(range[1], targetWidth, targetHeight, viewport);
  vtkTextMapper *maxTextMapper = this->LabelMappers[labelIdx];
  vtkActor2D *maxTextActor = this->LabelActors[labelIdx];

  int minmaxTextHeight = maxTextMapper->GetHeight(viewport);

  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    minTextMapper->GetTextProperty()->SetJustificationToLeft();
    minTextActor->SetPosition(0, 0);
    maxTextMapper->GetTextProperty()->SetJustificationToLeft();
    maxTextActor->SetPosition(0,
                  propSize[1] - titleSize[1] - this->TitleSpace - targetHeight);
    }
  else
    {
    minTextMapper->GetTextProperty()->SetJustificationToLeft();
    minTextActor->SetPosition(0, (  propSize[1] - titleSize[1]
                                  - this->BarHeight - 2*targetHeight
                                  - this->TitleSpace - this->LabelSpace ));
    maxTextMapper->GetTextProperty()->SetJustificationToRight();
    maxTextActor->SetPosition(propSize[0], (  propSize[1] - titleSize[1]
                                            - this->BarHeight - 2*targetHeight
                                            - this->TitleSpace
                                            - this->LabelSpace ));
    }

  // Figure out the precision to use based on the width of the scalar bar.
  if (!this->NumberOfLabels)
    {
    this->LabelHeight = 0;
    }
  else
    {
    // Figure out the dimensions we want for each label.  Target height already
    // determined but may need to be adjusted.  Target width needs to be set to
    // what is appropriate for middle labels, not just min/max.

    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      targetWidth = propSize[0]-this->BarWidth-this->LabelSpace;

      int maxHeight = (int)(this->BarHeight/this->NumberOfLabels);
      targetHeight = vtkstd::min(targetHeight, maxHeight);
      }
    else
      {
      targetWidth = (int)(propSize[0]*0.8/this->NumberOfLabels);

//       targetHeight = vtkstd::min(targetHeight, propSize[1]-this->BarHeight);
      }

    vtkstd::vector<double> ticks;
    if (isLogTable)
      {
      ticks = this->LogTickMarks(range, this->NumberOfLabels);
      }
    else
      {
      ticks = this->LinearTickMarks(range, this->NumberOfLabels);
      }

    VTK_CREATE(vtkCellArray, tickCells);
    tickCells->Allocate(tickCells->EstimateSize(ticks.size()*10, 2));

    VTK_CREATE(vtkPoints, tickPoints);
    tickPoints->Allocate(ticks.size()*20);

    for (int i = 0; i < static_cast<int>(ticks.size()); i++)
      {
      double val = ticks[i];
      double normVal;
      if (isLogTable)
        {
        normVal = (  (log10(val) - log10(range[0]))
                   / (log10(range[1]) - log10(range[0])) );
        }
      else
        {
        normVal = (val - range[0])/(range[1] - range[0]);
        }

      vtkTextMapper *textMapper = NULL;
      vtkActor2D *textActor = NULL;

      // Do not create the label if it is already represented in the min or max
      // label.
      if (   (val - 1e-6*MY_ABS(val+range[0]) > range[0])
          && (val + 1e-6*MY_ABS(val+range[1]) < range[1]))
        {
        labelIdx = this->CreateLabel(val, targetWidth, targetHeight, viewport);
        textMapper = this->LabelMappers[labelIdx];
        textActor = this->LabelActors[labelIdx];
        }

      if (this->Orientation == VTK_ORIENT_VERTICAL)
        {
        double x = this->BarWidth;
        double y = normVal*this->BarHeight + minmaxTextHeight;
        vtkIdType ids[2];
        ids[0] = tickPoints->InsertNextPoint(x - this->LabelSpace + 2, y, 0.0);
        ids[1] = tickPoints->InsertNextPoint(x + this->LabelSpace - 2, y, 0.0);
        tickCells->InsertNextCell(2, ids);
        if (textMapper != NULL)
          {
          int textSize[2];
          textMapper->GetSize(viewport, textSize);
          y -= textSize[1]/2;   // Adjust to center text.
          // Do not intersect min/max
          y = vtkstd::max(y, static_cast<double>(minmaxTextHeight));
          y = vtkstd::min(y, static_cast<double>(  propSize[1] - titleSize[1]
                                                 - this->TitleSpace
                                                 - minmaxTextHeight
                                                 - textSize[1]));
          textMapper->GetTextProperty()->SetJustificationToLeft();
          textActor->SetPosition(x + this->LabelSpace, y);
          }
        }
      else // this->Orientation == VTK_ORIENT_HORIZONTAL
        {
        double x = normVal*this->BarWidth;
        double y = (  propSize[1] - titleSize[1] - this->TitleSpace
                    - this->LabelHeight - this->LabelSpace );
        vtkIdType ids[2];
        ids[0] = tickPoints->InsertNextPoint(x, y - this->LabelSpace + 2, 0.0);
        ids[1] = tickPoints->InsertNextPoint(x, y + this->LabelSpace - 2, 0.0);
        tickCells->InsertNextCell(2, ids);
        if (textMapper != NULL)
          {
          textMapper->GetTextProperty()->SetJustificationToCentered();
          textActor->SetPosition(x, y + this->LabelSpace);
          }
        }
      }

    // Create minor tick marks.
    int numTicks = static_cast<int>(ticks.size());
    if (numTicks > 1)
      {
      // Decide how many (maximum) minor ticks we want based on how many pixels
      // are available.
      double fractionOfRange;
      if (isLogTable)
        {
        double tickDelta;
        tickDelta = log10(ticks[numTicks-1]) - log10(ticks[0]);
        fractionOfRange = (tickDelta)/(log10(range[1])-log10(range[0]));
        }
      else
        {
        double tickDelta;
        tickDelta = ticks[numTicks-1] - ticks[0];
        fractionOfRange = (tickDelta)/(range[1]-range[0]);
        }

      double pixelsAvailable;
      if (this->Orientation == VTK_ORIENT_VERTICAL)
        {
        pixelsAvailable = fractionOfRange*this->BarHeight;
        }
      else
        {
        pixelsAvailable = fractionOfRange*this->BarWidth;
        }
      int maxNumMinorTicks = vtkMath::Floor(pixelsAvailable/5);

      // This array lists valid minor to major tick ratios.
      const int minorRatios[] = {10, 5, 2, 1};
      const int numMinorRatios
        = static_cast<int>(sizeof(minorRatios)/sizeof(int));
      int minorRatio = 0;
      for (int r = 0; r < numMinorRatios; r++)
        {
        minorRatio = minorRatios[r];
        int numMinorTicks = (numTicks-1)*minorRatio;
        if (numMinorTicks <= maxNumMinorTicks) break;
        }

      // Add "fake" major ticks so that the minor ticks extend to bar ranges.
      double fakeMin, fakeMax;
      if (!isLogTable)
        {
        fakeMin = 2*ticks[0] - ticks[1];
        fakeMax = 2*ticks[numTicks-1] - ticks[numTicks-2];
        }
      else
        {
        fakeMin = pow(10.0, 2*log10(ticks[0]) - log10(ticks[1]));
        fakeMax = pow(10.0,
                      2*log10(ticks[numTicks-1]) - log10(ticks[numTicks-2]));
        }
      ticks.insert(ticks.begin(), fakeMin);
      ticks.insert(ticks.end(), fakeMax);
      numTicks = static_cast<int>(ticks.size());

      for (int i = 0; i < numTicks-1; i++)
        {
        double minorTickRange[2];
        minorTickRange[0] = ticks[i];  minorTickRange[1] = ticks[i+1];
        for (int j = 0; j < minorRatio; j++)
          {
          double val = (  ((minorTickRange[1]-minorTickRange[0])*j)/minorRatio
                        + minorTickRange[0]);
          double normVal;
          if (isLogTable)
            {
            normVal = (  (log10(val) - log10(range[0]))
                       / (log10(range[1]) - log10(range[0])) );
            }
          else
            {
            normVal = (val - range[0])/(range[1] - range[0]);
            }

          // Do not draw ticks out of range.
          if ((normVal < 0.0) || (normVal > 1.0)) continue;

          if (this->Orientation == VTK_ORIENT_VERTICAL)
            {
            double x = this->BarWidth;
            double y = normVal*this->BarHeight + minmaxTextHeight;
            vtkIdType ids[2];
            ids[0] = tickPoints->InsertNextPoint(x, y, 0.0);
            ids[1] = tickPoints->InsertNextPoint(x+this->LabelSpace-2, y, 0.0);
            tickCells->InsertNextCell(2, ids);
            }
          else // ths->Orientation == VTK_ORIENT_HORIZONTAL
            {
            double x = normVal*this->BarWidth;
            double y = (  propSize[1] - titleSize[1] - this->TitleSpace
                        - this->LabelHeight - this->LabelSpace );
            vtkIdType ids[2];
            ids[0] = tickPoints->InsertNextPoint(x, y, 0.0);
            ids[1] = tickPoints->InsertNextPoint(x, y+this->LabelSpace-2, 0.0);
            tickCells->InsertNextCell(2, ids);
            }
          }
        }
      }

    this->TickMarks->SetLines(tickCells);
    this->TickMarks->SetPoints(tickPoints);

    // "Mute" the color of the tick marks.
    double color[3];
    //this->TickMarksActor->GetProperty()->GetColor(color);
    this->LabelTextProperty->GetColor(color);
    vtkMath::RGBToHSV(color, color);
    if (color[2] > 0.5)
      {
      color[2] -= 0.2;
      }
    else
      {
      color[2] += 0.2;
      }
    vtkMath::HSVToRGB(color, color);
    this->TickMarksActor->GetProperty()->SetColor(color);
    }
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::PositionScalarBar(const int propSize[2],
                                            vtkViewport *viewport)
{
  // Get the size of the title.
  int titleSize[2];
  this->TitleMapper->GetSize(viewport, titleSize);

  // Determine bounds of scalar bar.
  double pLow[2], pHigh[2];
  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    pLow[0] = 0.0;
    pLow[1] = this->LabelHeight;
    pHigh[0] = this->BarWidth;
    pHigh[1] = this->BarHeight + this->LabelHeight;
    }
  else // this->Orientation == VTK_ORIENT_HORIZONTAL
    {
    pLow[0] = 0.0;
    pLow[1] = (  propSize[1] - titleSize[1] - this->TitleSpace
               - this->LabelHeight - this->LabelSpace - this->BarHeight );
    pHigh[0] = this->BarWidth;
    pHigh[1] = (  propSize[1] - titleSize[1] - this->TitleSpace
                - this->LabelHeight - this->LabelSpace );
    }

  // Set up points.
  VTK_CREATE(vtkPoints, points);
  points->SetDataTypeToFloat();
  points->SetNumberOfPoints(4);
  points->SetPoint(0, pLow[0],  pLow[1],  0.0);
  points->SetPoint(1, pHigh[0], pLow[1],  0.0);
  points->SetPoint(2, pHigh[0], pHigh[1], 0.0);
  points->SetPoint(3, pLow[0],  pHigh[1], 0.0);
  this->ScalarBar->SetPoints(points);

  // Set up polygons.
  VTK_CREATE(vtkCellArray, polys);
  polys->Allocate(polys->EstimateSize(1, 4));
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->ScalarBar->SetPolys(polys);

  // Set up texture coordinates.
  VTK_CREATE(vtkFloatArray, textCoords);
  textCoords->SetNumberOfComponents(2);
  textCoords->SetNumberOfTuples(4);
  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    textCoords->SetTuple2(0, 0, 0);
    textCoords->SetTuple2(1, 0, 0);
    textCoords->SetTuple2(2, 1, 0);
    textCoords->SetTuple2(3, 1, 0);
    }
  else // this->Orientation == VTK_ORIENT_HORIZONTAL
    {
    textCoords->SetTuple2(0, 0, 0);
    textCoords->SetTuple2(1, 1, 0);
    textCoords->SetTuple2(2, 1, 0);
    textCoords->SetTuple2(3, 0, 0);
    }
  this->ScalarBar->GetPointData()->SetTCoords(textCoords);
}

void vtkPVScalarBarActor::BuildScalarBarTexture()
{
  VTK_CREATE(vtkFloatArray, tmp);
  tmp->SetNumberOfTuples(COLOR_TEXTURE_MAP_SIZE);
  double *range = this->LookupTable->GetRange();
  int isLogTable = this->LookupTable->UsingLogScale();
  for (int i = 0; i < COLOR_TEXTURE_MAP_SIZE; i++)
    {
    double normVal = (double)i/(COLOR_TEXTURE_MAP_SIZE-1);
    double val;
    if (isLogTable)
      {
      double lval = log10(range[0]) + normVal*(log10(range[1])-log10(range[0]));
      val = pow(10.0,lval);
      }
    else
      {
      val = (range[1]-range[0])*normVal + range[0];
      }
    tmp->SetValue(i, val);
    }
  VTK_CREATE(vtkImageData, colorMapImage);
  colorMapImage->SetExtent(0, COLOR_TEXTURE_MAP_SIZE-1, 0, 0, 0, 0);
  colorMapImage->SetNumberOfScalarComponents(4);
  colorMapImage->SetScalarTypeToUnsignedChar();
  vtkDataArray *colors
    = this->LookupTable->MapScalars(tmp, VTK_COLOR_MODE_MAP_SCALARS, 0);
  colorMapImage->GetPointData()->SetScalars(colors);
  colors->Delete();

  this->ScalarBarTexture->SetInput(colorMapImage);
}
