/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPointLabelDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPointLabelDisplayProxy - Collect the pick data.
// .SECTION Description
// This class takes an input and collects the data for display in the UI.
// It is responsible for displaying the labels on the points.
// This display can work only in single process client mode as it does
// not have any compositer.

#ifndef __vtkSMPointLabelDisplayProxy_h
#define __vtkSMPointLabelDisplayProxy_h

#include "vtkSMConsumerDisplayProxy.h"

class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkSMPointLabelDisplayProxy : public vtkSMConsumerDisplayProxy
{
public:
  static vtkSMPointLabelDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMPointLabelDisplayProxy, vtkSMConsumerDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input. 
  void SetInput (vtkSMSourceProxy* input);

  // Description:
  // I have this funny looking AddInput instead of a simple
  // SetInput as I want to have an InputProperty for the input (rather than
  // a proxy property).
  virtual void AddInput(vtkSMSourceProxy* input, const char*, int);

  // Description:
  // This method updates the piece that has been assigned to this process.
  // Leads to a call to ForceUpdate on UpdateSuppressorProxy iff
  // GeometryIsValid==0;
  virtual void Update();

  //BTX
  // Description:
  // The Pick needs access to this to fill in the UI point values.
  // TODO: I have to find a means to get rid of this!!
  vtkUnstructuredGrid* GetCollectedData();
  //ETX
  
  // Description:
  // Accessor to the font size in the sub proxy.
  void SetFontSizeCM(int size);
  int GetFontSizeCM();
  
protected:
  vtkSMPointLabelDisplayProxy();
  ~vtkSMPointLabelDisplayProxy();

  void SetupPipeline();
  void SetupDefaults();

  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Marks for Update.
  virtual void InvalidateGeometryInternal(int useCache);
  
  vtkSMProxy* CollectProxy;
  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* MapperProxy;
  vtkSMProxy* ActorProxy;
  vtkSMProxy* TextPropertyProxy;
  int GeometryIsValid;
private:
  vtkSMPointLabelDisplayProxy(const vtkSMPointLabelDisplayProxy&); // Not implemented.
  void operator=(const vtkSMPointLabelDisplayProxy&); // Not implemented.
};


#endif
