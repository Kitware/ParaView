/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotActorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXYPlotActorProxy - proxy for vtkXYPlotActor.
// .SECTION Description
// This is a proxy for the XY plot actor. This is not a display proxy
// and hence cannot be added directly to a render module.

#ifndef __vtkSMXYPlotActorProxy_h
#define __vtkSMXYPlotActorProxy_h

#include "vtkSMProxy.h"
class vtkSMSourceProxy;
class vtkSMXYPlotActorProxyInternals;

class VTK_EXPORT vtkSMXYPlotActorProxy : public vtkSMProxy
{
public:
  static vtkSMXYPlotActorProxy* New();
  vtkTypeRevisionMacro(vtkSMXYPlotActorProxy, vtkSMProxy);
  void PrintSelf(ostream &os , vtkIndent indent);

  // Description:
  // Sets the input dataset to Plot.
  // Note that all the arrays and all the components of each of the arrays 
  // in the input will be plotted. 
  void AddInput(vtkSMSourceProxy* input, const char* method, int portIdx, 
    int hasMultipleInputs);

  // Description:
  // To remove the dataset for the Plot.
  void CleanInputs(const char* cleancommand);

  // Description:
  // Methods to set the position/position2 of the actor.
  void SetPosition(double x, double y);
  void SetPosition2(double x, double y);

  // Description:
  // Remove all the arrays selected for plotting.
  void RemoveAllArrayNames();

  // Description:
  // Add an array name to be plotted.
  void AddArrayName(const char* arrayname);

  // Description:
  // Method to push the property values onto server objects.
  virtual void UpdateVTKObjects();

protected:
  vtkSMXYPlotActorProxy();
  ~vtkSMXYPlotActorProxy();

  vtkSMXYPlotActorProxyInternals* Internals;
  int ArrayNamesModified;

  vtkSMSourceProxy* Input;
  void SetInput(vtkSMSourceProxy*);

  void SetupInputs();

private:
  vtkSMXYPlotActorProxy(const vtkSMXYPlotActorProxy&); // Not implemented.
  void operator=(const vtkSMXYPlotActorProxy&); // Not implemented.
};

#endif

