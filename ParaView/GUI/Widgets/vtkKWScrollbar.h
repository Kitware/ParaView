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

#include "vtkKWCoreWidget.h"

class KWWIDGETS_EXPORT vtkKWScrollbar : public vtkKWCoreWidget
{
public:
  static vtkKWScrollbar* New();
  vtkTypeRevisionMacro(vtkKWScrollbar,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

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

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked to change the view in the widget associated with
  // the scrollbar. 
  // The first argument is the object that will have the method called on it.
  // The second argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method
  // is evaluated as a simple command.
  virtual void SetCommand(vtkObject *object, const char *method);

protected:
  vtkKWScrollbar() {};
  ~vtkKWScrollbar() {};

private:
  vtkKWScrollbar(const vtkKWScrollbar&); // Not implemented
  void operator=(const vtkKWScrollbar&); // Not implemented
};


#endif



