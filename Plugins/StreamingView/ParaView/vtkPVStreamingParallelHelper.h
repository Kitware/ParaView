/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStreamingParallelHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVStreamingParallelHelper - class that synchronizes streaming
// drivers in in ParaView
// .SECTION Description
// This class uses ParaView's communicators and syncrhonizedX classes to
// do the communication necessary to keep parallel vtkStreamingDrivers
// working together.

#ifndef __vtkPVStreamingParallelHelper_h
#define __vtkPVStreamingParallelHelper_h

#include "vtkParallelStreamHelper.h"

class vtkPVSynchronizedRenderWindows;

class VTK_EXPORT vtkPVStreamingParallelHelper : public vtkParallelStreamHelper
{
public:
  vtkTypeMacro(vtkPVStreamingParallelHelper,vtkParallelStreamHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVStreamingParallelHelper *New();

  //Description:
  //A command that is called in parallel to make all processors agree on
  //the value of flag.
  void Reduce(bool &flag);

  //Description:
  // We use the parallel synchronized render windows as a conduit to communicators
  // and to query rendering mode, remote, client, tile etc.
  void SetSynchronizedWindows(vtkPVSynchronizedRenderWindows*);
  vtkGetMacro(SynchronizedWindows, vtkPVSynchronizedRenderWindows*);

//BTX
protected:
  vtkPVStreamingParallelHelper();
  ~vtkPVStreamingParallelHelper();

  vtkPVSynchronizedRenderWindows *SynchronizedWindows;

  enum {
    STREAMING_REDUCE_TAG = 838666
  };

private:
  vtkPVStreamingParallelHelper(const vtkPVStreamingParallelHelper&);  // Not implemented.
  void operator=(const vtkPVStreamingParallelHelper&);  // Not implemented.

//ETX
};

#endif
