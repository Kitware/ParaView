/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVPolyDataMapper - PV version of the mapper.
// .SECTION Description
// This subclass just adds the ability to select a portion of the input
// with an assignment.

#ifndef __vtkPVPolyDataMapper_h
#define __vtkPVPolyDataMapper_h

#include "vtkMesaPolyDataMapper.h"
#include "vtkPVAssignment.h"

class VTK_EXPORT vtkPVPolyDataMapper : public vtkMesaPolyDataMapper
{
public:
  static vtkPVPolyDataMapper* New();
  vtkTypeMacro(vtkPVPolyDataMapper, vtkMesaPolyDataMapper);
  
  // Description:
  // Paraviews way of telling the mapper what to display.
  vtkSetObjectMacro(Assignment, vtkPVAssignment);
  vtkGetObjectMacro(Assignment, vtkPVAssignment);

  // Description:
  // Update the input to the Mapper.
  void Update();
  
  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  float *GetBounds();
  
protected:
  vtkPVPolyDataMapper();
  ~vtkPVPolyDataMapper();
  vtkPVPolyDataMapper(const vtkPVPolyDataMapper&) {};
  void operator=(const vtkPVPolyDataMapper&) {};

  vtkPVAssignment *Assignment;
};

#endif

