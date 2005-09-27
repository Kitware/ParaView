/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositeDisplayProxy- a simple display proxy.
// .SECTION Description

#ifndef __vtkSMCompositeDisplayProxy_h
#define __vtkSMCompositeDisplayProxy_h

#include "vtkSMLODDisplayProxy.h"
class vtkPVLODPartDisplayInformation;
class VTK_EXPORT vtkSMCompositeDisplayProxy : public vtkSMLODDisplayProxy
{
public:
  static vtkSMCompositeDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMCompositeDisplayProxy, vtkSMLODDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enables or disables the collection filter.
  virtual void SetCollectionDecision(int val);
  vtkGetMacro(CollectionDecision, int);
  virtual void SetLODCollectionDecision(int val);
  vtkGetMacro(LODCollectionDecision, int);
  //BTX
  enum MoveMode {PASS_THROUGH=0, COLLECT, CLONE};
  enum Server { CLIENT=0, DATA_SERVER, RENDER_SERVER};
  //ETX

  virtual void SetOrderedCompositing(int val);
  vtkGetMacro(OrderedCompositing, int);

  virtual void SetOrderedCompositingTree(vtkSMProxy *tree);
  vtkGetObjectMacro(OrderedCompositingTree, vtkSMProxy);

  //BTX
  // Description:
  // This is a little different than superclass 
  // because it updates the geometry if it is out of date.
  //  Collection flag gets turned off if it needs to update.
  virtual vtkPVLODPartDisplayInformation* GetLODInformation();
  //ETX

  // Description:
  // Overridden to set up ordered compositing correctly.
  virtual void SetVisibility(int visible);

  virtual void Update();
  virtual void UpdateDataToDistribute();

  virtual void CacheUpdate(int idx, int total);

protected:
  vtkSMCompositeDisplayProxy();
  ~vtkSMCompositeDisplayProxy();

  virtual void SetupPipeline();
  virtual void SetupDefaults();

  virtual void SetupVolumePipeline();
  virtual void SetupVolumeDefaults();


  virtual void CreateVTKObjects(int numObjects);
  void SetupCollectionFilter(vtkSMProxy* collectProxy);

  vtkSMProxy* CollectProxy;
  vtkSMProxy* LODCollectProxy;
  vtkSMProxy* VolumeCollectProxy;

  vtkSMProxy* DistributorProxy;
  vtkSMProxy* LODDistributorProxy;
  vtkSMProxy* VolumeDistributorProxy;

  vtkSMProxy* DistributorSuppressorProxy;
  vtkSMProxy* LODDistributorSuppressorProxy;
  vtkSMProxy* VolumeDistributorSuppressorProxy;

  int CollectionDecision;
  int LODCollectionDecision;

  int DistributedGeometryIsValid;
  int DistributedLODGeometryIsValid;
  int DistributedVolumeGeometryIsValid;

  int OrderedCompositing;

  vtkSMProxy* OrderedCompositingTree;

  virtual void RemoveGeometryFromCompositingTree();
  virtual void AddGeometryToCompositingTree();

  virtual void InvalidateGeometryInternal();

private:
  vtkSMCompositeDisplayProxy(const vtkSMCompositeDisplayProxy&); // Not implemented.
  void operator=(const vtkSMCompositeDisplayProxy&); // Not implemented.
};

#endif

