/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerTimeSteps.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerTimeSteps - Server-side helper for vtkSMTimeStepsInformationHelper.
// .SECTION Description
// Gets the TIME_STEPS() key values and sends it to the client.

#ifndef __vtkPVServerTimeSteps_h
#define __vtkPVServerTimeSteps_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkAlgorithm;
class vtkPVServerTimeStepsInternals;


class VTK_EXPORT vtkPVServerTimeSteps : public vtkPVServerObject
{
public:
  static vtkPVServerTimeSteps* New();
  vtkTypeMacro(vtkPVServerTimeSteps, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Gets the TIME_STEPS() key values and sends it to the client.
  const vtkClientServerStream& GetTimeSteps(vtkAlgorithm*);

protected:
  vtkPVServerTimeSteps();
  ~vtkPVServerTimeSteps();

  // Internal implementation details.
  vtkPVServerTimeStepsInternals* Internal;

private:
  vtkPVServerTimeSteps(const vtkPVServerTimeSteps&); // Not implemented
  void operator=(const vtkPVServerTimeSteps&); // Not implemented
};

#endif
