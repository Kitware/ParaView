/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeRenderModule - Uses composite manager and collection.
// .SECTION Description
// This render manager is for parallel execution using MPI.
// It creates a special vtkPVPartDisplay (todo) that collects small
// geometry for local rendering.  It also manages reduction factor
// which renders and composites a small window then magnifies for final
// display.

#ifndef __vtkPVCompositeRenderModule_h
#define __vtkPVCompositeRenderModule_h

#include "vtkPVLODRenderModule.h"

class VTK_EXPORT vtkPVCompositeRenderModule : public vtkPVLODRenderModule
{
public:
  static vtkPVCompositeRenderModule* New();
  vtkTypeRevisionMacro(vtkPVCompositeRenderModule,vtkPVLODRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetCompositeThreshold(float);
  vtkGetMacro(CompositeThreshold, float);

  // Description:
  // Renders using Still/FullRes or interactive/LODs
  virtual void StillRender();
  virtual void InteractiveRender();

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

  // Description:
  void SetUseCompositeWithFloat(int val);
  void SetUseCompositeWithRGBA(int val);
  virtual void SetUseCompositeCompression(int val);

  int GetCompositeID() { return this->CompositeID.ID; }
  
  // Description:
  // These use the total memory size of the visible
  // geoemtry and decimated LOD to make a collection decision.
  // I would like to move this method into a rendering module.  
  // It resides here for the moment because vtkPVWindow has a list of sources.
  int MakeCollectionDecision();
  int MakeLODCollectionDecision();

  // Description:
  // For picking the center of rotation.
  virtual float GetZBufferValue(int x, int y);

  // Subclass create their own vtkSMPartDisplay object by
  // implementing this method.
  virtual vtkSMPartDisplay* CreatePartDisplay();
  
protected:
  vtkPVCompositeRenderModule();
  ~vtkPVCompositeRenderModule();

  // Computes the reduction factor to use in compositing.
  void ComputeReductionFactor();
  int ReductionFactor;
  int SquirtLevel;

  int LocalRender;

  int CollectionDecision;
  int LODCollectionDecision;

  float CompositeThreshold;

  vtkClientServerID CompositeID;

private:
  vtkPVCompositeRenderModule(const vtkPVCompositeRenderModule&); // Not implemented
  void operator=(const vtkPVCompositeRenderModule&); // Not implemented
};


#endif
