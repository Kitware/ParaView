/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBlockDeliveryRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBlockDeliveryRepresentationProxy
// .SECTION Description
// This is a representation proxy for spreadsheet view which can be used to show
// output of algorithms  producing vtkDataObject or subclasses.

#ifndef __vtkSMBlockDeliveryRepresentationProxy_h
#define __vtkSMBlockDeliveryRepresentationProxy_h

#include "vtkSMClientDeliveryRepresentationProxy.h"

class vtkSMSourceProxy;
class vtkDataObject;

class VTK_EXPORT vtkSMBlockDeliveryRepresentationProxy : 
  public vtkSMClientDeliveryRepresentationProxy
{
public:
  static vtkSMBlockDeliveryRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMBlockDeliveryRepresentationProxy, 
    vtkSMClientDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the data that was collected to the client. Overridden to return the
  // data in the cache, if any. This method does not update the representation
  // if data is obsolete, use GetBlockOutput() instead.
  virtual vtkDataObject* GetOutput();

  // Description:
  // Retuns the dataset for the current block.
  // This may call update if the block is not available in the cache, or the
  // cache is obsolete.
  virtual vtkDataObject* GetBlockOutput();

  // Description:
  // Get/Set the current block number.
  vtkSetMacro(Block, vtkIdType);
  vtkGetMacro(Block, vtkIdType);

  // Description:
  // Set the cache size as the maximum number of blocks to cache at a given
  // time. When cache size exceeds this number, the least-recently-accessed
  // block(s) will be discarded.
  vtkSetMacro(CacheSize, vtkIdType);
  vtkGetMacro(CacheSize, vtkIdType);

  // Description:
  // Called to update the Representation. 
  // Overridden to forward the update request to the strategy if any. 
  // If subclasses don't use any strategy, they may want to override this
  // method.
  // Fires vtkCommand:StartEvent and vtkCommand:EndEvent and start and end of
  // the update if it is indeed going to cause some pipeline execution.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Returns if the representation's input has changed since most recent
  // Update(). Overridden to forward the request to the strategy, if any. If
  // subclasses don't use any strategy, they may want to override this method.
  virtual bool UpdateRequired();


  // Description:
  // Clean all cached data. It's usually not required to use this method
  // explicitly, whenever the input is modified, the cache is automatically
  // cleaned.
  void CleanCache();

  // Description:
  // Overridden to update the CleanCacheOnUpdate flag. 
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

//BTX
protected:
  vtkSMBlockDeliveryRepresentationProxy();
  ~vtkSMBlockDeliveryRepresentationProxy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Overridden to set default property values.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Create the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  vtkSMSourceProxy* BlockFilter;
  vtkSMSourceProxy* Reduction;

  // Active block id.
  vtkIdType Block;
  vtkIdType CacheSize;

  bool CleanCacheOnUpdate;
private:
  vtkSMBlockDeliveryRepresentationProxy(const vtkSMBlockDeliveryRepresentationProxy&); // Not implemented
  void operator=(const vtkSMBlockDeliveryRepresentationProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

