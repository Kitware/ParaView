/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkViewLayout - used by vtkSMViewLayoutProxy.
// .SECTION Description
// vtkViewLayout is the server-side object corresponding to
// vtkSMViewLayoutProxy.

#ifndef __vtkViewLayout_h
#define __vtkViewLayout_h

#include "vtkObject.h"

class VTK_EXPORT vtkViewLayout : public vtkObject
{
public:
  static vtkViewLayout* New();
  vtkTypeMacro(vtkViewLayout, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void ResetTileDisplay();
  void ShowOnTileDisplay(unsigned int);

//BTX
protected:
  vtkViewLayout();
  ~vtkViewLayout();

private:
  vtkViewLayout(const vtkViewLayout&); // Not implemented
  void operator=(const vtkViewLayout&); // Not implemented
//ETX
};

#endif
