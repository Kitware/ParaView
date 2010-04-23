/*=========================================================================

  Program:   ParaView
  Module:    vtkSMHardwareSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMHardwareSelector
// .SECTION Description
//

#ifndef __vtkSMHardwareSelector_h
#define __vtkSMHardwareSelector_h

#include "vtkSMProxy.h"

class vtkSelection;
class VTK_EXPORT vtkSMHardwareSelector : public vtkSMProxy
{
public:
  static vtkSMHardwareSelector* New();
  vtkTypeMacro(vtkSMHardwareSelector, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSelection* Select();

//BTX
protected:
  vtkSMHardwareSelector();
  ~vtkSMHardwareSelector();

  void StartSelectionPass();

private:
  vtkSMHardwareSelector(const vtkSMHardwareSelector&); // Not implemented
  void operator=(const vtkSMHardwareSelector&); // Not implemented
//ETX
};

#endif

