/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationBatchHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationBatchHelper - helper class for doing animation in batch mode
// .SECTION Description

#ifndef __vtkPVAnimationBatchHelper_h
#define __vtkPVAnimationBatchHelper_h

#include "vtkObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMDomain;
class vtkSMProperty;

class VTK_EXPORT vtkPVAnimationBatchHelper : public vtkObject
{
public:
  static vtkPVAnimationBatchHelper* New();
  vtkTypeRevisionMacro(vtkPVAnimationBatchHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetAnimationValueInBatch(ofstream* file, vtkSMDomain *domain,
                                vtkSMProperty *property,
                                vtkClientServerID sourceID, int idx,
                                double value);

protected:
  vtkPVAnimationBatchHelper() {}
  ~vtkPVAnimationBatchHelper() {}

private:
  vtkPVAnimationBatchHelper(const vtkPVAnimationBatchHelper&); // Not implemented
  void operator=(const vtkPVAnimationBatchHelper&); // Not implemented
};

#endif
