/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVColorByProcess.h
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

#ifndef __vtkPVColorByProcess_h
#define __vtkPVColorByProcess_h

#include "vtkPVDataSetToDataSetFilter.h"
#include "vtkColorByProcess.h"

class VTK_EXPORT vtkPVColorByProcess : public vtkPVDataSetToDataSetFilter
{
public:
  static vtkPVColorByProcess* New();
  vtkTypeMacro(vtkPVColorByProcess, vtkPVDataSetToDataSetFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();

  // Description:
  // Gets the filter cast to the correct type.
  vtkColorByProcess* GetColorByProcess();

  // Description:
  // I do not want to use "GetGlobalController", so the filter needs to
  // get the controller from the application.
  void SetApplication(vtkKWApplication *app);

protected:
  vtkPVColorByProcess();
  ~vtkPVColorByProcess() {};
  vtkPVColorByProcess(const vtkPVColorByProcess&) {};
  void operator=(const vtkPVColorByProcess&) {};
  
};

#endif
