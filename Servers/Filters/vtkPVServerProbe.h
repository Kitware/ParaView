/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerProbe.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerArraySelection - Server-side helper for vtkPVProbe.
// .SECTION Description

#ifndef __vtkPVServerProbe_h
#define __vtkPVServerProbe_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPProbeFilter;

class VTK_EXPORT vtkPVServerProbe : public vtkPVServerObject
{
public:
  static vtkPVServerProbe* New();
  vtkTypeRevisionMacro(vtkPVServerProbe, vtkPVServerObject);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Get the polydata output from vtkPProbeFilter.
  void SendPolyDataOutput(vtkPProbeFilter*);
  
protected:
  vtkPVServerProbe();
  ~vtkPVServerProbe();
  
private:
  vtkPVServerProbe(const vtkPVServerProbe&);  // Not implemented
  void operator=(const vtkPVServerProbe&);  // Not implemented
};

#endif
