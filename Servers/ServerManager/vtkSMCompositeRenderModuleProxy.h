/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositeRenderModuleProxy -  Render module supporting LODs.
// .SECTION Description
// This render manager is for parallel execution using MPI.
// It creates a special vtkPVPartDisplay (todo) that collects small
// geometry for local rendering.  It also manages reduction factor
// which renders and composites a small window then magnifies for final
// display.

#ifndef __vtkSMCompositeRenderModuleProxy_h
#define __vtkSMCompositeRenderModuleProxy_h

#include "vtkSMLODRenderModuleProxy.h"
// We could have very well derrived this from vtkSMRenderModuleProxy, but hey!

class vtkSMCompositeDisplayProxy;
class vtkSMDisplayProxy;

class VTK_EXPORT vtkSMCompositeRenderModuleProxy : public vtkSMLODRenderModuleProxy
{
public:
  static vtkSMCompositeRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMCompositeRenderModuleProxy, vtkSMLODRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  vtkSetMacro(CompositeThreshold, double);
  vtkGetMacro(CompositeThreshold, double);

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite (or client server).
  vtkSetMacro(ReductionFactor, int);
  vtkGetMacro(ReductionFactor, int);

  // Description:
  // Squirt is a hybrid run length encoding and bit reduction compression
  // algorithm that is used to compress images for transmition from the
  // server to client.  Value of 0 disabled all compression.  Level zero is just
  // run length compression with no bit compression (lossless).
  vtkSetMacro(SquirtLevel, int);
  vtkGetMacro(SquirtLevel, int);

  virtual void InteractiveRender();
  virtual void StillRender();
protected:
  vtkSMCompositeRenderModuleProxy();
  ~vtkSMCompositeRenderModuleProxy();

  virtual void CreateVTKObjects(int numObjects);
  // Computes the reduction factor to use in compositing.
  void ComputeReductionFactor();
  int ReductionFactor;
  int SquirtLevel;

  int LocalRender;

  int CollectionDecision;
  int LODCollectionDecision;

  double CompositeThreshold;
  
  vtkSMProxy* CompositeManagerProxy;

  // Description:
  // Subclasses must decide what type of CompositeManagerProxy they need.
  // This method is called to make that decision. Subclasses are expected to
  // add the CompositeManagerProxy as a SubProxy named "CompositeManager".
  virtual void CreateCompositeManager() { }; //TODO: pure virtual.

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();

  // Indicates if we should locally render.
  // Flag stillRender is set when this decision is to be made during StillRender
  // else it's 0 (for InteractiveRender);
  virtual int GetLocalRenderDecision(unsigned long totalMemory, int stillRender);

  // Convenience method to set CollectionDecition on DisplayProxy.
  void SetCollectionDecision(vtkSMCompositeDisplayProxy* pDisp, int decision);
  
  // Convenience method to set LODCollectionDecision on DisplayProxy.
  void SetLODCollectionDecision(vtkSMCompositeDisplayProxy* pDisp, int decision);
  
  // Convenience method to set ImageReductionFactor on Composity Proxy.
  // Note that this message is sent only to the client.
  void SetImageReductionFactor(vtkSMProxy* compositor, int factor);

  // Convenience method to set Squirt Level on Composite Proxy.
  // Note that this message is sent only to the client.
  void SetSquirtLevel(vtkSMProxy* compositor, int level);
  
  // Convenience method to set Use Compositing on COmposite Proxy.
  // Note that this message is sent only to the client.
  void SetUseCompositing(vtkSMProxy* p, int flag);

private:
  vtkSMCompositeRenderModuleProxy(const vtkSMCompositeRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMCompositeRenderModuleProxy&); // Not implemented.
};

#endif

