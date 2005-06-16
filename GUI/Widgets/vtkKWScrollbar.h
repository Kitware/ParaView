/*=========================================================================

  Module:    vtkKWScrollbar.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWScrollbar - a simple scrollbar
// .SECTION Description
// The core scrollbar


#ifndef __vtkKWScrollbar_h
#define __vtkKWScrollbar_h

#include "vtkKWWidget.h"

class KWWIDGETS_EXPORT vtkKWScrollbar : public vtkKWWidget
{
public:
  static vtkKWScrollbar* New();
  vtkTypeRevisionMacro(vtkKWScrollbar,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set/Get the orientation type.
  // For widgets that can lay themselves out with either a horizontal or
  // vertical orientation, such as scrollbars, this option specifies which 
  // orientation should be used. 
  // Valid constants can be found in vtkKWTkOptions::OrientationType.
  virtual void SetOrientation(int);
  virtual int GetOrientation();
  virtual void SetOrientationToHorizontal() 
    { this->SetOrientation(vtkKWTkOptions::OrientationHorizontal); };
  virtual void SetOrientationToVertical() 
    { this->SetOrientation(vtkKWTkOptions::OrientationVertical); };

protected:
  vtkKWScrollbar() {};
  ~vtkKWScrollbar() {};

private:
  vtkKWScrollbar(const vtkKWScrollbar&); // Not implemented
  void operator=(const vtkKWScrollbar&); // Not implemented
};


#endif



