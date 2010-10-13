/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

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
  // This needs to be called on all instances of vtkGeometryRepresentation when
  // the input is modified. This is essential since the geometry filter does not
  // have any real-input on the client side which messes with the Update
  // requests.
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
