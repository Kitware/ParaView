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

#include "vtkKWCompositeWidget.h"

class vtkKWCanvas;
class vtkKWProgressGaugeInternals;

class KWWidgets_EXPORT vtkKWProgressGauge : public vtkKWCompositeWidget
{
public:
  static vtkKWProgressGauge* New();
  vtkTypeRevisionMacro(vtkKWProgressGauge,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the percentage displayed for the primary progress gauge. 
  // This number is clamped between 0.0 and 100.0.
  virtual void SetValue(double value);
  virtual double GetValue();
  
  // Description:
  // Set/Get the percentage displayed for the primary (ranked 0) and
  // the secondary progress gauges. If rank = 0, calling this method is
  // the same as calling the SetValue method.
  // This number is clamped between 0.0 and 100.0.
  // All progress gauges are stacked vertically on top of each other, with the
  // lower rank at the bottom. Space for the primary gauge (ranked 0) is
  // always allocated. It is not for secondary gauges which value is 0.0
  // unless a higher rank gauge is != 0.0.
  virtual void SetNthValue(int rank, double value);
  virtual double GetNthValue(int rank);
  
  // Description:
  // Set/Get the width and height of the widget.
  // The height parameter is ignored if ExpandHeight is set to On.
  virtual void SetWidth(int width);
  vtkGetMacro(Width, int);
  virtual void SetHeight(int height);
  vtkGetMacro(Height, int);

  // Description:
  // Set/Get if the height of the gauge should be automatically adjusted
  // to fill the available vertical space. The widget should be packed
  // accordingly to expand automatically. Check MinimumHeight too.
  vtkBooleanMacro(ExpandHeight, int);
  virtual void SetExpandHeight(int);
  vtkGetMacro(ExpandHeight, int);

  // Description:
  // Set/Get the minimum height of the widget.
  // This value is ignored if ExpandHeight is set to Off. If set to On,
  // the height computed from the available vertical space will not be any
  // smaller than this minimum height. 
  virtual void SetMinimumHeight(int height);
  vtkGetMacro(MinimumHeight, int);
  
  // Description:
  // Set/Get the color of the progress bar, the default is blue.
  virtual void SetBarColor(double r, double g, double b);
  virtual void SetBarColor(double rgb[3])
    { this->SetBarColor(rgb[0], rgb[1], rgb[2]); }
  vtkGetVectorMacro(BarColor,double,3);

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ConfigureCallback();

protected:
  vtkKWProgressGauge();
  ~vtkKWProgressGauge();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  virtual void Redraw();

  int Width;
  int Height;
  int MinimumHeight;
  double BarColor[3];
  double Value;
  int ExpandHeight;

  vtkKWCanvas *Canvas;

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWProgressGaugeInternals *Internals;
  //ETX

private:
  vtkKWProgressGauge(const vtkKWProgressGauge&); // Not implemented
  void operator=(const vtkKWProgressGauge&); // Not implemented
};


#endif

