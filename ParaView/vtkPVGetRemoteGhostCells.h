/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGetRemoteGhostCells.h
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

#ifndef __vtkPVGetRemoteGhostCells_h
#define __vtkPVGetRemoteGhostCells_h

#include "vtkPVPolyDataToPolyDataFilter.h"

class vtkPVApplication;
class vtkGetRemoteGhostCells;


class VTK_EXPORT vtkPVGetRemoteGhostCells : public vtkPVPolyDataToPolyDataFilter
{
public:
  static vtkPVGetRemoteGhostCells* New();
  vtkTypeMacro(vtkPVGetRemoteGhostCells, vtkPVPolyDataToPolyDataFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();

  // Description:
  // The class is overriding this method becuase the VTK filter needs
  // the controller.
  void SetApplication(vtkKWApplication *app);
  
protected:
  vtkPVGetRemoteGhostCells();
  ~vtkPVGetRemoteGhostCells() {};
  vtkPVGetRemoteGhostCells(const vtkPVGetRemoteGhostCells&) {};
  void operator=(const vtkPVGetRemoteGhostCells&) {};

  vtkGetRemoteGhostCells *GetRemoteGhostCells();
};

#endif
