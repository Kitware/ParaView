/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentationStrategy - strategy for representation proxies that
// encapsulates lod/parallel pipelines.
// .SECTION Description
// vtkSMRepresentationStrategy is an abstract superclass that encapsulates the 
// lod/parallel pipelines that make data visible to the user.
// Because of this class representation implementations do not need to 
// implement a pipeline for handling parallel rendering. Instead they simply
// ask the view proxy to which they are added to create a strategy and it 
// will create the correct strategy for the representation's data type.

#ifndef __vtkSMRepresentationStrategy_h
#define __vtkSMRepresentationStrategy_h

#include "vtkSMProxy.h"

class vtkInformation;
class vtkPVDataInformation;
class vtkPVInformation;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMRepresentationStrategy : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMRepresentationStrategy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects filters/sinks to an input. 
  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput()=0;

  // Description:
  // Get low resolution output.
  virtual vtkSMSourceProxy* GetLODOutput()=0;

  // Description:
  // Get the data information. If EnableLOD is true and ViewInformation 
  // indicates that the rendering decision was to use LOD, then
  // this method will return the data information for the 
  // LOD data displayed otherwise non-lod data information 
  // will be returned.
  // This method on the strategy does not update the pipeline before getting 
  // the data information i.e. it will returns the current data information.
  unsigned long GetDisplayedMemorySize();

  // Description:
  // Returns data information from the full res pipeline. Note that this
  // returns the current data information irrespective of whether the pipeline
  // is up-to-date.
  virtual unsigned long GetFullResMemorySize()
    { return this->DataSize; }

  // Description:
  // Returns data information from the lod pipeline. This returns the current
  // data information irrespective of whether the pipeline is up-to-date.
  virtual unsigned long GetLODMemorySize()
    { return this->LODDataSize; }

  // Description:
  // Must be set if the representation makes use of the LOD pipeline provided 
  // by the strategy. false by default, implying that the representation only 
  // displays the high resolution data.
  // This flag can only be changed before the objects are created. Changing it
  // after that point will raise errors.
  vtkGetMacro(EnableLOD, bool);
  virtual void SetEnableLOD(bool enable)
    {
    if (this->EnableLOD != enable)
      {
      if (this->ObjectsCreated )
        {
        vtkErrorMacro("Cannot change EnableLOD flag after objects have "
          "been created");
        }
      else
        {
        this->EnableLOD = enable;
        this->Modified();
        }
      }
    }

  // Description:
  // Must be set if the representation wants the strategy to enable caching.
  // When set to true the stratergy will use cache when the ViewInformation 
  // indicates so. true by default.
  vtkSetMacro(EnableCaching, bool);
  vtkGetMacro(EnableCaching, bool);

  // Description:
  // Helper is used to determine the current LOD decision.
  void SetViewInformation(vtkInformation*);

  // Description:
  // Returns if the strategy is not up-to-date. This happens
  // when any proxy upstream was modified since the last Update()
  // on the strategy's displayed pipeline.
  // Note that this reply may change if the decision to use
  // LOD changes.
  virtual bool UpdateRequired();

  // Description:
  // Updates the display pipeline if update is required.
  // A representation always updates the full-res pipeline if it needs any
  // updating i.e. irrepective of if LOD is being used. On the other hand, LOD
  // pipeline is updated only when the ViewInformation indicates that LOD is
  // enabled.
  // When an update is required, this method invokes StartEvent and EndEvent to
  // mark the start and end of update request.
  virtual void Update();

  // Description:
  // Updates part of the display pipeline. This updates the part of the pipeline
  // before any data movement/redistribution happens. This is useful to gather
  // information about data sizes etc from the server without actually
  // undertaking any expensive data transfers.
  // Similar to Update(), the LOD subpipline is updated only if LOD is used.
  virtual void UpdateDataInformation();

  // Description:
  // Returns the current data information for the represented data.
  // One can call UpdateDataInformation() before calling this method to get the
  // data information from update pipeline. This method does not update the
  // pipeline.
  vtkGetObjectMacro(RepresentedDataInformation, vtkPVDataInformation);

  // Description:
  // Returns if the strategy is currently using LOD 
  // i.e. (UseLOD && EnableLOD).
  bool GetUseLOD();

  // Description:
  // Returns if the strategy is currently using cache 
  // (UseCache && EnableCaching).
  bool GetUseCache();

  // Description:
  // When set to true, and EnableLOD is true, every Update() request will also
  // update the LODPipeline irrespective of whether we are currently using LOD.
  // Default value is false.
  vtkSetMacro(KeepLODPipelineUpdated, bool);
  vtkGetMacro(KeepLODPipelineUpdated, bool);

  // Description:
  // Overridden to clear data valid flags.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

//BTX
protected:
  vtkSMRepresentationStrategy();
  ~vtkSMRepresentationStrategy();

// Description:
  // Overridden to call BeginCreateVTKObjects() and EndCreateVTKObjects()
  // before creating the objects.
  virtual void CreateVTKObjects();

  // Description:
  // Called before objects are created.
  virtual void BeginCreateVTKObjects();

  // Description:
  // Called after objects are created.
  virtual void EndCreateVTKObjects(){}

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport) =0;

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called only if EnableLOD
  // flag is true.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input, int outputport) =0;

  // Description:
  // Gather the information of the displayed data (non-LOD).
  // Update the part of the pipeline needed to gather full information
  // and then gather that information. 
  virtual void GatherInformation(vtkPVInformation*) = 0;

  // Description:
  // Gather the information of the displayed data (lod);
  // Update the part of the pipeline needed to gather full information
  // and then gather that information. 
  // Default implementation simply calls GatherInformation().
  virtual void GatherLODInformation(vtkPVInformation* info)
    { this->GatherInformation(info); }

  // Description:
  // Update the LOD pipeline. Subclasses must override this method
  // to provide their own implementation and then call the superclass
  // to ensure that various flags are updated correctly.
  // This method need not worry about caching, since LODPipeline is never used
  // when caching is enabled.
  virtual void UpdateLODPipeline();

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Subclasses must override this method
  // to provide their own implementation and then call the superclass
  // to ensure that various flags are updated correctly.
  virtual void UpdatePipeline();

  // Description:
  // Invalidates the LOD pipeline.
  virtual void InvalidateLODPipeline();

  // Description:
  // Invalidates the full resolution pipeline.
  virtual void InvalidatePipeline();

  // Description:
  // Creates a connection between the producer and the consumer
  // using "Input" property. Subclasses can use this to build
  // pipelines.
  void Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname="Input", int outputport=0);

  // Description:
  // Called when ever the view information changes.
  // The strategy should update it's state based on the current view information
  // provided the information object.
  virtual void ProcessViewInformation();

  // Description:
  // Called when the ViewInformation is modified to set whether 
  // the strategy should use LOD pipeline if
  // enabled. This does not invalidate any pipeline.
  vtkSetMacro(UseLOD, bool);

  // Description:
  // Called when the ViewInformation is modified to set whether the strategy 
  // should use cache if enabled. This does not invalidate any pipeline.
  void SetUseCache(bool);

  // Description:
  // Called when the ViewInformation is modified to set LOD resolution.
  // Set the LOD resolution. This invalidates the LOD pipeline if the resolution
  // has indeed changed.
  virtual void SetLODResolution(int resolution)
    {
    if (this->LODResolution != resolution)
      {
      this->LODResolution = resolution;
      this->InvalidateLODPipeline();
      }
    }

  // Description:
  // Returns true is data is valid.
  virtual bool GetDataValid()
    { return this->DataValid; }

  // Description:
  // Returns true is LOD data is valid.
  virtual bool GetLODDataValid()
    { return this->LODDataValid; }

  bool UseCache;
  bool UseLOD;
  double CacheTime;
  bool EnableCaching;

  bool LODDataValid;
  bool DataValid;

  bool InformationValid;
  bool LODInformationValid;

  int LODResolution;

  // When set to true, LODPipeline is always udpated with the full-res pipeline
  // (unless EnableLOD is false).
  bool KeepLODPipelineUpdated;

  vtkInformation* ViewInformation;

  vtkSMSourceProxy* Input;
  int OutputPort;

private:
  // Description:
  // Cleans cache. Called in InvalidatePipeline() and InvalidateLODPipeline().
  // Cache is not cleaned if caching is enabled since it implies that we are
  // building cache.
  void CleanCacheIfObsolete();

  // Flag used to avoid unnecessary "RemoveAllCaches" requests being set to the
  // server.
  bool SomethingCached;

private:
  vtkSMRepresentationStrategy(const vtkSMRepresentationStrategy&); // Not implemented
  void operator=(const vtkSMRepresentationStrategy&); // Not implemented

  vtkSMSourceProxy* CacheKeeper;
  bool EnableLOD;

  vtkCommand* Observer;

  vtkPVDataInformation* RepresentedDataInformation;
  void SetRepresentedDataInformation(vtkPVDataInformation*);
  unsigned long DataSize;
  unsigned long LODDataSize;


//ETX
};

#endif

