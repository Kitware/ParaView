/*=========================================================================

  Program:   ParaView
  Module:    vtkLocalConeSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLocalConeSource - Example ParaView Plugin Source
// .SECTION Description
// vtkLocalConeSource is a subclass of vtkConeSource that adds no
// functionality but is sufficient to show an example ParaView plugin.

#ifndef __vtkLocalConeSource_h
#define __vtkLocalConeSource_h

#include "vtkConeSource.h"

#include "vtkPVLocal.h" // VTK_PVLocal_EXPORT

class VTK_PVLocal_EXPORT vtkLocalConeSource : public vtkConeSource
{
public:
  vtkTypeRevisionMacro(vtkLocalConeSource,vtkConeSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkLocalConeSource* New();

protected:
  vtkLocalConeSource();
  ~vtkLocalConeSource();

private:
  vtkLocalConeSource(const vtkLocalConeSource&);  // Not implemented.
  void operator=(const vtkLocalConeSource&);  // Not implemented.
};

#endif
