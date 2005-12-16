/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientSideDataProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMClientSideDataProxy - Proxy for getting data on the client.
// .SECTION Description
// Copies/consolidates data from the servers, so it can be accessed
// by client-side code.
// .SECTION See Also
// vtkSMXYPlotDisplayProxy

#ifndef __vtkSMClientSideDataProxy_h
#define __vtkSMClientSideDataProxy_h

#include "vtkSMConsumerDisplayProxy.h"

class vtkDataSet;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMClientSideDataProxy : public vtkSMConsumerDisplayProxy
{
public:
  static vtkSMClientSideDataProxy* New();
  vtkTypeRevisionMacro(vtkSMClientSideDataProxy, vtkSMConsumerDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // I have this funny looking AddInput instead of a simple
  // SetInput as I want to have an InputProperty for the input (rather than
  // a proxy property).
  virtual void AddInput(vtkSMSourceProxy* input, const char*,  int );

  //BTX
  // Description:
  // The Probe needs access to this to fill in the UI point values.
  // Only needed when probing one point only.
  // TODO: I have to find a means to get rid of this!!
  vtkDataSet *GetCollectedData();
  //ETX
  
  // Description:
  // This method updates the piece that has been assigned to this process.
  // Leads to a call to ForceUpdate on UpdateSuppressorProxy iff
  // GeometryIsValid==0;
  virtual void Update();
  
  // Description:
  // Marks for Update.
  virtual void InvalidateGeometry();

  // Description:
  // Chains to superclass and calls InvalidateGeometry().
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

protected:
  vtkSMClientSideDataProxy();
  ~vtkSMClientSideDataProxy();
  
  virtual void CreateVTKObjects(int numObjects);

  void SetupPipeline();
  void SetupDefaults();
  
  // This is not reference counted. 
  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* CollectProxy;

  int GeometryIsValid; // Flag indicating is Update must call ForceUpdate.
  
  int PolyOrUGrid;
  
private:
  vtkSMClientSideDataProxy(const vtkSMClientSideDataProxy&); // Not implemented.
  void operator=(const vtkSMClientSideDataProxy&); // Not implemented.
};

#endif
