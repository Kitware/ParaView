/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSegmentedProgressGauge.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkKWSegmentedProgressGauge - a segmented progress bar widget
// .SECTION Description
// vtkKWSegmentedProgressGauge is a widget to display progress for tasks
// that can be logically broken into (up to 4) segments rather than
// continuous progress (e.g., rendering LODs).  Each segment has a different
// color, varying between red (earliest segment) to green (last segment).

#ifndef __vtkKWSegmentedProgressGauge_h
#define __vtkKWSegmentedProgressGauge_h

#include "vtkKWWidget.h"

class VTK_EXPORT vtkKWSegmentedProgressGauge : public vtkKWWidget
{
public:
  // Description:
  // Standard New and type methods
  static vtkKWSegmentedProgressGauge *New();
  vtkTypeRevisionMacro(vtkKWSegmentedProgressGauge, vtkKWWidget);
  
  // Description:
  // Print information about this object
  void PrintSelf(ostream &os, vtkIndent);
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Set the percentage complete for a particular segment.  All earlier
  // segments are considered completed.
  void SetValue(int segment, int value);

  // Description:
  // Set the number of segments in the progress gauge
  vtkSetClampMacro(NumberOfSegments, int, 1, 4);
  vtkGetMacro(NumberOfSegments, int);
  
  // Description:
  // Set the width and height of the progress gauge
  vtkSetMacro(Width, int);
  vtkGetMacro(Width, int);
  vtkSetMacro(Height, int);
  vtkGetMacro(Height, int);
  
protected:
  vtkKWSegmentedProgressGauge();
  ~vtkKWSegmentedProgressGauge();

  vtkKWWidget *ProgressFrame;
  vtkKWWidget *ProgressCanvas;

  int NumberOfSegments;
  int Width;
  int Height;

  int Segment;
  int Value;
  
private:
  vtkKWSegmentedProgressGauge(const vtkKWSegmentedProgressGauge&);  //Not implemented
  void operator=(const vtkKWSegmentedProgressGauge&);  //Not implemented
};

#endif
