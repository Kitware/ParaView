/*=========================================================================

  Module:    vtkKWProgressGauge.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWProgressGauge - a horizontal progress bar widget
// .SECTION Description
// A simple widget used for displaying a progress bar with a percent value
// text in the center of the widget.

#ifndef __vtkKWProgressGauge_h
#define __vtkKWProgressGauge_h

#include "vtkKWWidget.h"

class vtkKWCanvas;

class KWWIDGETS_EXPORT vtkKWProgressGauge : public vtkKWWidget
{
public:
  static vtkKWProgressGauge* New();
  vtkTypeRevisionMacro(vtkKWProgressGauge,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the width and height of the widget
  // The height parameter is ignored is ExpandHeight is set.
  virtual void SetWidth(int width);
  vtkGetMacro(Width, int);
  virtual void SetHeight(int height);
  vtkGetMacro(Height, int);

  // Description:
  // Set/Get if the height of the gauge should be automatically adjusted
  // to fill the available vertical space. The widget should be packed
  // accordingly to expand automatically.
  vtkBooleanMacro(ExpandHeight, int);
  virtual void SetExpandHeight(int);
  vtkGetMacro(ExpandHeight, int);
  
  // Description:
  // Set/Get the percentage displayed. This number is clamped to be betwen
  // 0.0 and 100.0
  virtual void SetValue(double value);
  vtkGetMacro(Value, double);
  
  // Description:
  // Set the color of the progress bar, the default is blue.
  virtual void SetBarColor(double r, double g, double b);
  virtual void SetBarColor(double rgb[3])
    { this->SetBarColor(rgb[0], rgb[1], rgb[2]); }
  vtkGetVectorMacro(BarColor,double,3);

  // Description:
  // Callbacks
  virtual void ConfigureCallback();

protected:
  vtkKWProgressGauge();
  ~vtkKWProgressGauge();

  virtual void Redraw();

  int Width;
  int Height;
  double BarColor[3];
  double Value;
  int ExpandHeight;

  vtkKWCanvas *Canvas;

private:
  vtkKWProgressGauge(const vtkKWProgressGauge&); // Not implemented
  void operator=(const vtkKWProgressGauge&); // Not implemented
};


#endif

