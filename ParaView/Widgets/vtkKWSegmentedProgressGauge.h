/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

