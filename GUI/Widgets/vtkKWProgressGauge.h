/*=========================================================================

  Module:    vtkKWProgressGauge.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWProgressGauge - a progress bar widget
// .SECTION Description
// A simple widget used for displaying a progress bar with a percent value
// text in the center of the widget.

#ifndef __vtkKWProgressGauge_h
#define __vtkKWProgressGauge_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWProgressGauge : public vtkKWWidget
{
public:
  static vtkKWProgressGauge* New();
  vtkTypeRevisionMacro(vtkKWProgressGauge,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set and get the length and width of the widget
  vtkSetMacro(Length, int);
  vtkGetMacro(Length, int);
  void SetHeight(int height);
  vtkGetMacro(Height, int);
  
  // Description:
  // Set the percentage displayed.  This number is forced to be in
  // the range 0 to 100.
  void SetValue(int value);
  
  // Description:
  // Set the color of the progress bar, the default is blue.
  vtkSetStringMacro(BarColor);
  vtkGetStringMacro(BarColor);

protected:
  vtkKWProgressGauge();
  ~vtkKWProgressGauge();
private:
  int Length;
  int Height;
  char* BarColor;
  int Value;
private:
  vtkKWProgressGauge(const vtkKWProgressGauge&); // Not implemented
  void operator=(const vtkKWProgressGauge&); // Not implemented
};


#endif

