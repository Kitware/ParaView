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

  // Description:
  // Causes redistribution to occur.
  virtual void InvalidateDistributedGeometry();

  // Description:
  // Returns true if some distributed geometry is still valid.
  virtual int IsDistributedGeometryValid();

  // Description:
  // If the ordered compositing tree changed, just invalidate the distributed
  // geometry.
  virtual void MarkModified(vtkSMProxy* modifiedProxy); 

  virtual void Update();
  virtual void UpdateDistributedGeometry();

  // Description:
  // This method returns if the Update() or UpdateDistributedGeometry()
  // calls will actually lead to an Update. This is used by the render module
  // to decide if it can expect any pipeline updates.
  virtual int UpdateRequired();

  virtual void CacheUpdate(int idx, int total);

protected:
  vtkSMCompositeDisplayProxy();
  ~vtkSMCompositeDisplayProxy();

  virtual void SetupPipeline();
  virtual void SetupDefaults();

  virtual void SetupVolumePipeline();
  virtual void SetupVolumeDefaults();

  virtual void SetInputInternal(vtkSMSourceProxy* input);


  virtual void CreateVTKObjects(int numObjects);
  void SetupCollectionFilter(vtkSMProxy* collectProxy);

  // The generic set method.
  void SetOrderedCompositingTreeInternal(vtkSMProxy* tree);

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

private:
  vtkSMCompositeDisplayProxy(const vtkSMCompositeDisplayProxy&); // Not implemented.
  void operator=(const vtkSMCompositeDisplayProxy&); // Not implemented.
};

#endif

