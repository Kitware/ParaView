/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataRepresentation
// .SECTION Description
// vtkPVDataRepresentation adds some ParaView specific API to data
// representations.
// .SECTION See Also
// vtkPVDataRepresentationPipeline

#ifndef __vtkPVDataRepresentation_h
#define __vtkPVDataRepresentation_h

#include "vtkDataRepresentation.h"

class vtkInformationRequestKey;

class VTK_EXPORT vtkPVDataRepresentation : public vtkDataRepresentation
{
public:
  vtkTypeMacro(vtkPVDataRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  // Overridden to skip processing when visibility if off.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // This is one of the most important functions. In VTK pipelines, it's very
  // easy for the pipeline to decide when it needs to re-execute.
  // vtkAlgorithm::Update() can go up the entire pipeline to see if any filters
  // MTime changed (among other things) and if so, it can re-execute the pipeline.
  // However in case of representations, the real input connection may only be
  // present on the data-server nodes. In that case the
  // vtkPVDataRepresentation::RequestData() will only get called on the
  // data-server nodes. That means that representations won't be able to any
  // data-delivery in RequestData(). We'd need some other mechanisms to
  // synchronize data-delivery among processes. To avoid that conundrum, the
  // vtkSMRepresentationProxy calls MarkModified() on all processes whenever any
  // filter in the pipeline is modified. In this method, the
  // vtkPVDataRepresentation subclasses should ensure that they mark all
  // delivery related filters dirty in their RequestData to ensure they execute
  // then next time they are updated. The vtkPVDataRepresentation also uses a
  // special executive which avoids updating the representation unless
  // MarkModified() was called since the last Update(), thus acting as a
  // update-suppressor.
  virtual void MarkModified();

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool val)
    { this->Visibility = val; }
  vtkGetMacro(Visibility, bool);

  // Description:
  // Returns the data object that is rendered from the given input port.
  virtual vtkDataObject* GetRenderedDataObject(int vtkNotUsed(port))
    { return this->GetInputDataObject(0, 0); }

  // Description:
  // Set the update time.
  virtual void SetUpdateTime(double time);
  vtkGetMacro(UpdateTime, double);

  // Description:
  // Set whether the UpdateTime is valid.
  vtkGetMacro(UpdateTimeValid, bool);

  // Description:
  // This controls when to use cache and when using cache, what cached value to
  // use for the next update. This class using a special executive so that is a
  // data is cached, then it does not propagate any pipeline requests upstream.
  // These ivars are updated by vtkPVView::Update() based on the corresponding
  // variable values on the vtkPVView itself.
  virtual void SetUseCache(bool use)
    { this->UseCache = use; }
  virtual void SetCacheKey(double val)
    { this->CacheKey = val; }

  // Description:
  // Typically UseCache and CacheKey are updated by the View and representations
  // cache based on what the view tells it. However in some cases we may want to
  // force a representation to cache irrespective of the view (e.g. comparative
  // views). In which case these ivars can up set. If ForcedCacheKey is true, it
  // overrides UseCache and CacheKey. Instead, ForcedCacheKey is used.
  virtual void SetForcedCacheKey(double val)
    { this->ForcedCacheKey = val; }
  virtual void SetForceUseCache(bool val)
    { this->ForceUseCache = val; }

  // Description:
  // Returns whether caching is used and what key to use when caching is
  // enabled.
  virtual double GetCacheKey()
    { return this->ForceUseCache? this->ForcedCacheKey : this->CacheKey; }
  virtual bool GetUseCache()
    { return this->ForceUseCache || this->UseCache; }

  // Description:
  // Called by vtkPVDataRepresentationPipeline to see if using cache is valid
  // and will be used for the update. If so, it bypasses all pipeline passes.
  // Subclasses should override IsCached(double) to indicate if a particular
  // entry is cached.
  bool GetUsingCacheForUpdate();

  vtkGetMacro(NeedUpdate,  bool);

  // Description:
  // Making these methods public. When constructing composite representations,
  // we need to call these methods directly on internal representations.
  virtual bool AddToView(vtkView* view)
    { return this->Superclass::AddToView(view); }
  virtual bool RemoveFromView(vtkView* view)
    { return this->Superclass::RemoveFromView(view); }

  // Description:
  // Retrieves an output port for the input data object at the specified port
  // and connection index. This may be connected to the representation's
  // internal pipeline.
  // Overridden to use vtkPVTrivialProducer instead of vtkTrivialProducer
  virtual vtkAlgorithmOutput* GetInternalOutputPort()
    { return this->GetInternalOutputPort(0); }
  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port)
    { return this->GetInternalOutputPort(port, 0); }
  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port, int conn);

//BTX
protected:
  vtkPVDataRepresentation();
  ~vtkPVDataRepresentation();

  // Description:
  // Subclasses should override this method when they support caching to
  // indicate if the particular key is cached. Default returns false.
  virtual bool IsCached(double cache_key)
    { (void)cache_key; return false; }

  // Description:
  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Overridden to invoke vtkCommand::UpdateDataEvent.
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  virtual int RequestUpdateExtent(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  double UpdateTime;
  bool UpdateTimeValid;
private:
  vtkPVDataRepresentation(const vtkPVDataRepresentation&); // Not implemented
  void operator=(const vtkPVDataRepresentation&); // Not implemented

  bool Visibility;
  bool UseCache;
  bool ForceUseCache;
  double CacheKey;
  double ForcedCacheKey;
  bool NeedUpdate;

//ETX
};

#endif
