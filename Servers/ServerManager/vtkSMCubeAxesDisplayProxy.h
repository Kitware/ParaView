/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCubeAxesDisplayProxy - Collect the pick data.
// .SECTION Description
// This class takes an input and collects the data for display in the UI.
// It is responsible for displaying the labels on the points.

#ifndef __vtkSMCubeAxesDisplayProxy_h
#define __vtkSMCubeAxesDisplayProxy_h


#include "vtkSMConsumerDisplayProxy.h"

class vtkDataSet;
class vtkPVDataInformation;
class vtkProp;
class vtkSMProxy;
class vtkProperty;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;
class vtkSMRenderModuleProxy;


class VTK_EXPORT vtkSMCubeAxesDisplayProxy : public vtkSMConsumerDisplayProxy
{
public:
  static vtkSMCubeAxesDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMCubeAxesDisplayProxy, vtkSMConsumerDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Called when setting input using the Input property.
  // Internally calls SetInput.
  virtual void AddInput(vtkSMSourceProxy* input, const char*, int);

  // Description:
  // Connects the parts data to the plot actor.
  // All point data arrays are ploted for now.
  // Data must already be updated.
  virtual void SetInput(vtkSMProxy* input);

  // Description:
  // Turns visibility on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  // Description:
  // PVSource calls this when it gets modified.
  virtual void InvalidateGeometryInternal(int useCache);

  // Description:
  // Chains to superclass and calls InvalidateGeometry()
  virtual void MarkModified(vtkSMProxy* modifiedProxy); 

protected:
  vtkSMCubeAxesDisplayProxy();
  ~vtkSMCubeAxesDisplayProxy();
  
  virtual void RemoveAllCaches();
  int NumberOfCaches;
  double **Caches;

  int GeometryIsValid;
  int Visibility;

  vtkSMRenderModuleProxy* RenderModuleProxy;
  vtkSMProxy* CubeAxesProxy;
  vtkSMSourceProxy* Input;

  virtual void CreateVTKObjects(int num);

  vtkSMCubeAxesDisplayProxy(const vtkSMCubeAxesDisplayProxy&); // Not implemented
  void operator=(const vtkSMCubeAxesDisplayProxy&); // Not implemented
};

#endif
