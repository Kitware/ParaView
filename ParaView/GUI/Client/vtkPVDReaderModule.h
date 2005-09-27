/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDReaderModule - 
// .SECTION Description

#ifndef __vtkPVDReaderModule_h
#define __vtkPVDReaderModule_h

#include "vtkPVAdvancedReaderModule.h"

class vtkPVScale;

class VTK_EXPORT vtkPVDReaderModule : public vtkPVAdvancedReaderModule
{
public:
  static vtkPVDReaderModule* New();
  vtkTypeRevisionMacro(vtkPVDReaderModule, vtkPVAdvancedReaderModule);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  virtual int Finalize(const char* fname);
  
  // Description:
  // Get the number of time steps that can be provided by this reader.
  // Timesteps are available either from an animation file or from a
  // time-series of files as detected by the file entry widget.
  // Returns 0 if time steps are not available, and the number of
  // timesteps otherwise.
  virtual int GetNumberOfTimeSteps();
  
  // Description:
  // Set the time step that should be provided by the reader.  This
  // value is ignored unless GetNumberOfTimeSteps returns 1 or more.
  virtual void SetRequestedTimeStep(int);
  
protected:
  vtkPVDReaderModule();
  ~vtkPVDReaderModule();
  
private:
  vtkPVDReaderModule(const vtkPVDReaderModule&); // Not implemented
  void operator=(const vtkPVDReaderModule&); // Not implemented
};

#endif
