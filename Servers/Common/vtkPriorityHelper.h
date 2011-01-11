/*=========================================================================

  Program:   ParaView
  Module:    vtkPriorityHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPriorityHelper - partially updates the input
// .SECTION Description
// When the servermanager drives the data processing pipeline, for example,
// when filling in the information tab, the streaming View plugin
// adds this to pipeline so that it can prevent whole data updates.

#ifndef __vtkPriorityHelper_h
#define __vtkPriorityHelper_h

#include "vtkObject.h"

class vtkAlgorithmOutput;
class vtkAlgorithm;
class vtkDataObject;

class VTK_EXPORT vtkPriorityHelper : public vtkObject
{
public:
  static vtkPriorityHelper* New();
  vtkTypeMacro(vtkPriorityHelper, vtkObject);

  // Descrition:
  // Tell it what filter you want to control.
  void SetInputConnection(vtkAlgorithmOutput *input);

  // Description:
  // Tell it what piece you want to get out of that filter.
  int SetSplitUpdateExtent(int port, //input filter's n'th output port
                           int processors, //parallel rank
                           int numProcessors, //parallel number of processes
                           int pass, //streaming pass
                           int numPasses, //streaming number of passes
                           double resolution); //streaming resolution

  // Description:
  // Get the data produced.
  vtkDataObject *GetDataObject();

  // Description:
  // Update the pipeline.
  void Update();

protected:
  vtkPriorityHelper();
  ~vtkPriorityHelper();

  vtkAlgorithm *Input;
  int Port;

private:
  vtkPriorityHelper(const vtkPriorityHelper&); // Not implemented
  void operator=(const vtkPriorityHelper&); // Not implemented
};

#endif
