/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkForceTime_h_
#define vtkForceTime_h_

#include "vtkDataObjectAlgorithm.h"

class VTK_EXPORT vtkForceTime : public vtkDataObjectAlgorithm
{
public :
  static vtkForceTime *New();
  vtkTypeMacro(vtkForceTime, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Replace the pipeline time by this one.
  vtkSetMacro(ForcedTime, double);
  vtkGetMacro(ForcedTime, double);

  // Description:
  // Use the ForcedTime. If disabled, use usual pipeline time.
  vtkSetMacro(IgnorePipelineTime, int);
  vtkGetMacro(IgnorePipelineTime, int);
  vtkBooleanMacro(IgnorePipelineTime, int);

protected:
  vtkForceTime();
  virtual ~vtkForceTime();

  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  double ForcedTime;
  int IgnorePipelineTime;

private:
  vtkForceTime(const vtkForceTime&);  // Not implemented.
  void operator=(const vtkForceTime&);  // Not implemented.
};

#endif //vtkForceTime_h_
