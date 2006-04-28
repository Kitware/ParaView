/*=========================================================================

  Module:    vtkKWSeparator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSeparator - a separator widget.
// .SECTION Description
// A simple separator widget that can be used to separate widgets
// using a simple horizontal or vertical line.
// .SECTION See Also
// vtkKWFrame
 
#ifndef __vtkKWSeparator_h
#define __vtkKWSeparator_h

#include "vtkKWFrame.h"

class KWWidgets_EXPORT vtkKWSeparator : public vtkKWFrame
{
public:
  static vtkKWSeparator* New();
  vtkTypeRevisionMacro(vtkKWSeparator, vtkKWFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the orientation of the separator.
  //BTX
  enum 
  {
    OrientationHorizontal = 0,
    OrientationVertical   = 1
  };
  //ETX
  virtual void SetOrientation(int);
  vtkGetMacro(Orientation, int);
  virtual void SetOrientationToHorizontal()
    { this->SetOrientation(vtkKWSeparator::OrientationHorizontal); };
  virtual void SetOrientationToVertical() 
    { this->SetOrientation(vtkKWSeparator::OrientationVertical); };

  // Description:
  // Set/Get the thickness of the separator.
  // Do not use the superclass's SetWidth and SetHeight method to set the
  // thickness.
  virtual void SetThickness(int);
  vtkGetMacro(Thickness, int);
  
protected:
  vtkKWSeparator();
  ~vtkKWSeparator() {};

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int Orientation;
  int Thickness;

  // Description:
  // Update the aspect of the widget
  virtual void UpdateAspect();

private:
  vtkKWSeparator(const vtkKWSeparator&); // Not implemented
  void operator=(const vtkKWSeparator&); // Not implemented
};


#endif



