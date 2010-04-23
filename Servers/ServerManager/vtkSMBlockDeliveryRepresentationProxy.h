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

#include "vtkSMDataRepresentationProxy.h"

class vtkSMSourceProxy;
class vtkDataObject;
class vtkSMClientDeliveryStrategyProxy;

class VTK_EXPORT vtkSMBlockDeliveryRepresentationProxy : 
  public vtkSMDataRepresentationProxy
{
public:
  static vtkSMBlockDeliveryRepresentationProxy* New();
  vtkTypeMacro(vtkSMBlockDeliveryRepresentationProxy,
    vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the data for the given block. This may fetch the data to the client,
  // if it's not already present.
  virtual vtkDataObject* GetOutput(vtkIdType block);

  // Description:
  // Indicates if the block is available on the client.
  virtual bool IsAvailable(vtkIdType blockid);

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
  // Clean all cached data. It's usually not required to use this method
  // explicitly, whenever the input is modified, the cache is automatically
  // cleaned.
  void CleanCache();

  // Description:
  // Returns the number of blocks that are needed to fetch the entire input
  // dataset given the current block size.
  vtkIdType GetNumberOfRequiredBlocks();

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
  virtual bool CreatePipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Ensures that the block of data is available on the client.
  void Fetch(vtkIdType block);

  vtkSMSourceProxy* PreProcessor;
  vtkSMSourceProxy* Streamer;
  vtkSMSourceProxy* Reduction;

  bool CacheDirty;

  vtkIdType CacheSize;
  unsigned int CompositeDataSetIndex;

  vtkSMRepresentationStrategy* UpdateStrategy;
  vtkSMClientDeliveryStrategyProxy* DeliveryStrategy;
private:
  vtkSMBlockDeliveryRepresentationProxy(const vtkSMBlockDeliveryRepresentationProxy&); // Not implemented
  void operator=(const vtkSMBlockDeliveryRepresentationProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

