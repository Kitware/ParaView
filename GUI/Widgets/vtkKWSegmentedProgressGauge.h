/*=========================================================================

  Module:    vtkKWSegmentedProgressGauge.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSegmentedProgressGauge - a segmented progress bar widget
// .SECTION Description
// vtkKWSegmentedProgressGauge is a widget to display progress for tasks
// that can be logically broken into (up to 10) segments rather than
// continuous progress (e.g., rendering LODs).  Each segment has a different
// color that can be independently set.

#ifndef __vtkKWSegmentedProgressGauge_h
#define __vtkKWSegmentedProgressGauge_h

#include "vtkKWWidget.h"

class vtkKWFrame;
class vtkKWCanvas;

class VTK_EXPORT vtkKWSegmentedProgressGauge : public vtkKWWidget
{
public:
  // Description:
  // Standard New and type methods
  static vtkKWSegmentedProgressGauge *New();
  vtkTypeRevisionMacro(vtkKWSegmentedProgressGauge, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Set the percentage complete for a particular segment.  All earlier
  // segments are considered completed.
  void SetValue(int segment, int value);

  // Description:
  // Set the number of segments in the progress gauge
  void SetNumberOfSegments(int number);
  vtkGetMacro(NumberOfSegments, int);
  
  // Description:
  // Set the width and height of the progress gauge
  vtkSetMacro(Width, int);
  vtkGetMacro(Width, int);
  vtkSetMacro(Height, int);
  vtkGetMacro(Height, int);

  // Description:
  // Set/Get the color for a particular segment.
  void SetSegmentColor( int index, float r, float g, float b );
  void SetSegmentColor( int index, float color[3] )
    {this->SetSegmentColor( index, color[0], color[1], color[2] );}
  void GetSegmentColor( int index, float color[3] );
      
protected:
  vtkKWSegmentedProgressGauge();
  ~vtkKWSegmentedProgressGauge();

  vtkKWFrame *ProgressFrame;
  vtkKWCanvas *ProgressCanvas;

  int NumberOfSegments;
  int Width;
  int Height;

  float SegmentColor[10][3];
  
  int Segment;
  int Value;
  
private:
  vtkKWSegmentedProgressGauge(const vtkKWSegmentedProgressGauge&);  //Not implemented
  void operator=(const vtkKWSegmentedProgressGauge&);  //Not implemented
};

#endif

