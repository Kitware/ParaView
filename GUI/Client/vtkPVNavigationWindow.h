/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNavigationWindow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVNavigationWindow - Widget for PV sources and their inputs and outputs
// .SECTION Description
// vtkPVNavigationWindow is a specialized ParaView widget used for
// displaying a local presentation of the underlying pipeline. It
// allows the user to navigate by clicking on the appropriate tags.

#ifndef __vtkPVNavigationWindow_h
#define __vtkPVNavigationWindow_h

#include "vtkPVSourcesNavigationWindow.h"

class vtkKWApplication;
class vtkKWWidget;
class vtkPVSource;
class vtkKWMenu;

class VTK_EXPORT vtkPVNavigationWindow : public vtkPVSourcesNavigationWindow
{
public:
  static vtkPVNavigationWindow* New();
  vtkTypeRevisionMacro(vtkPVNavigationWindow,vtkPVSourcesNavigationWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVNavigationWindow();
  ~vtkPVNavigationWindow();

  // This method actually does everything
  virtual void ChildUpdate(vtkPVSource*);

private:
  vtkPVNavigationWindow(const vtkPVNavigationWindow&); // Not implemented
  void operator=(const vtkPVNavigationWindow&); // Not Implemented
};


#endif


