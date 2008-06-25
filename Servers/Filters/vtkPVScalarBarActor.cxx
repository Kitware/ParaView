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

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkPVScalarBarActor.h"

#include "vtkObjectFactory.h"
#include "vtkTextActor.h"
#include "vtkScalarsToColors.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#include <vtkstd/algorithm>

#define MY_ABS(x)       ((x) < 0 ? -(x) : (x))

//=============================================================================
vtkCxxRevisionMacro(vtkPVScalarBarActor, "1.5");
vtkStandardNewMacro(vtkPVScalarBarActor);

//=============================================================================
vtkPVScalarBarActor::vtkPVScalarBarActor()
{
  this->AspectRatio = 20.0;
  this->AutomaticLabelFormat = 1;
}

vtkPVScalarBarActor::~vtkPVScalarBarActor()
{
}

void vtkPVScalarBarActor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AspectRatio: " << this->AspectRatio << endl;
  os << indent << "AutomaticLabelFormat: " << this->AutomaticLabelFormat <<endl;
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::AllocateAndSizeLabels(int *labelSize, 
                                                int *propSize,
                                                vtkViewport *viewport,
                                                double *range)
{
  labelSize[0] = labelSize[1] = 0;

  this->TextMappers = new vtkTextMapper * [this->NumberOfLabels];
  this->TextActors = new vtkActor2D * [this->NumberOfLabels];

  char format[512];
  char string[1024];

  double fontScaling = vtkTextActor::GetFontScale(viewport);

  int barWidth, barHeight;
  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    barHeight = (int)(0.86*propSize[1]);
    barWidth = (int)(barHeight/this->AspectRatio);
    }
  else
    {
    barWidth = propSize[0];
    barHeight = (int)(barWidth/this->AspectRatio);
    }
  
  // TODO: this should be optimized, maybe by keeping a list of
  // allocated mappers, in order to avoid creation/destruction of
  // their underlying text properties (i.e. each time a mapper is
  // created, text properties are created and shallow-assigned a font size
  // which value might be "far" from the target font size).

  // is this a vtkLookupTable or a subclass of vtkLookupTable 
  // with its scale set to log
  int isLogTable = this->LookupTable->UsingLogScale();

  // Figure out the precision to use based on the width of the scalar bar.
  if (this->NumberOfLabels)
    {
    // Create the first text mapper.  We are going to use it to figure out font
    // sizes.
    this->TextMappers[0] = vtkTextMapper::New();
    this->TextMappers[0]->GetTextProperty()->ShallowCopy(
                                                       this->LabelTextProperty);

    // Figure out the dimensions we want for each label.
    int targetWidth, targetHeight;

    // Initialize the height with the requested hight from the font (which
    // should be about the same for all mappers).
    this->TextMappers[0]->SetInput("0");
    targetHeight = (int)(fontScaling*this->TextMappers[0]->GetHeight(viewport));

    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      targetWidth = propSize[0]-barWidth;

      int maxHeight = (int)(0.86*propSize[1]/this->NumberOfLabels);
      targetHeight = vtkstd::min(targetHeight, maxHeight);
      }
    else
      {
      targetWidth = (int)(propSize[0]*0.8/this->NumberOfLabels);

//       targetHeight = vtkstd::min(targetHeight, propSize[1]-barHeight);
      }

    if (this->AutomaticLabelFormat)
      {
      // Get the maximum number of characters we can comfortably put in each
      // label without making it smaller.
      int targetStringLength;
      for (targetStringLength = 3; targetStringLength<100; targetStringLength++)
        {
        vtkstd::fill_n(string, targetStringLength, '0');
        string[targetStringLength] = '\0';
        this->TextMappers[0]->SetInput(string);
        this->TextMappers[0]->SetConstrainedFontSize(viewport,
                                                     VTK_INT_MAX, targetHeight);
        int expectedWidth = this->TextMappers[0]->GetWidth(viewport);
        if (expectedWidth > targetWidth) break;
        }
      targetStringLength--;

      // Find the limits of the exponents.
      double lowExp  = (range[0] == 0.0) ? 1.0 : log10(MY_ABS(range[0]));
      double highExp = (range[1] == 0.0) ? 1.0 : log10(MY_ABS(range[1]));

      if (   (vtkstd::min(lowExp, highExp) < -4.0)
          || (vtkstd::max(lowExp, highExp) > targetStringLength-2) )
        {
        // Use exponential formating
        sprintf(format, "%%-#0.%de", vtkstd::max(1, targetStringLength-7));
        }
      else
        {
        // Use floating point formating
        sprintf(format, "%%-#0.%dg", targetStringLength-2);
        }
      }
    else
      {
      sprintf(format, "%s", this->LabelFormat);
      }

    for (int i = 0; i < this->NumberOfLabels; i++)
      {
      if (i != 0)
        {
        this->TextMappers[i] = vtkTextMapper::New();
        }

      double val;
      if ( isLogTable )
        {
        double lval;
        if (this->NumberOfLabels > 1)
          {
          lval = log10(range[0]) + (double)i/(this->NumberOfLabels-1) *
            (log10(range[1])-log10(range[0]));
          }
        else
          {
          lval = log10(range[0]) + 0.5*(log10(range[1])-log10(range[0]));
          }
        val = pow(10.0,lval);
        }
      else
        {
        if (this->NumberOfLabels > 1)
          {
          val = range[0] + 
            (double)i/(this->NumberOfLabels-1) * (range[1]-range[0]);
          }
        else
          {
          val = range[0] + 0.5*(range[1]-range[0]);
          }
        }

      // Potential of buffer overrun (onto the stack) here.
      sprintf(string, format, val);
      this->TextMappers[i]->SetInput(string);

      // Shallow copy here so that the size of the label prop is not affected by
      // the automatic adjustment of its text mapper's size (i.e. its mapper's
      // text property is identical except for the font size which will be
      // modified later). This allows text actors to share the same text
      // property, and in that case specifically allows the title and label text
      // prop to be the same.
      this->TextMappers[i]->GetTextProperty()->ShallowCopy(
                                                       this->LabelTextProperty);

      this->TextActors[i] = vtkActor2D::New();
      this->TextActors[i]->SetMapper(this->TextMappers[i]);
      this->TextActors[i]->SetProperty(this->GetProperty());
      this->TextActors[i]->GetPositionCoordinate()->
        SetReferenceCoordinate(this->PositionCoordinate);
      }

    vtkTextMapper::SetMultipleConstrainedFontSize(viewport, 
                                                  VTK_INT_MAX,//targetWidth, 
                                                  targetHeight,
                                                  this->TextMappers,
                                                  this->NumberOfLabels,
                                                  labelSize);
    }

  // Adjust the size the superclass things the widget is so that the bar always
  // has the desired aspect ratio.
  if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
    propSize[0] = barWidth + labelSize[0] + 4;
    }
  else
    {
    propSize[1] = (int)(barHeight/0.4);
    }
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::SizeTitle(int *titleSize, int *vtkNotUsed(propSize), 
                                    vtkViewport *viewport)
{
  titleSize[0] = titleSize[1] = 0;

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
  this->TitleMapper->GetSize(viewport, titleSize);
}
