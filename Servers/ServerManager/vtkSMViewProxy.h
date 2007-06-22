/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMViewProxy - Superclass for all view proxies.
// .SECTION Description
// vtkSMViewProxy is a superclass for all view proxies. A view proxy
// abstracts the logic to take one or more representation proxies and show then
// in some viewport such as vtkRenderWindow.
// This class may directly be used as the view proxy for views that do all the
// rendering work at the GUI level.
// .SECTION Events
// \li vtkCommand::StartEvent(callData: int:0) -- start of StillRender().
// \li vtkCommand::EndEvent(callData: int:0) -- end of StillRender().
// \li vtkCommand::StartEvent(callData: int:1) -- start of InteractiveRender().
// \li vtkCommand::EndEvent(callData: int:1) -- end of InteractiveRender().

#ifndef __vtkSMViewProxy_h
#define __vtkSMViewProxy_h

#include "vtkSMProxy.h"

class vtkCollection;
class vtkCommand;
class vtkInformation;
class vtkInformationDoubleKey;
class vtkInformationIntegerKey;
class vtkSMRepresentationProxy;
class vtkSMRepresentationStrategy;

class VTK_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeRevisionMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Keys used to specify view rendering requirements.
  static vtkInformationIntegerKey* USE_CACHE();
  static vtkInformationDoubleKey* CACHE_TIME();

  // Description:
  // Adds a representation proxy to this view. 
  virtual void AddRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes a representation proxy from this view.
  virtual void RemoveRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes all added representations from this view.
  // Simply calls RemoveRepresentation() on all added representations 
  // one by one.
  void RemoveAllRepresentations();

  // Description:
  // Updates the data pipelines for all visible representations.
  virtual void UpdateAllRepresentations();

  // Description:
  // Renders the view using full resolution.
  // Internally calls 
  // \li UpdateAllDisplays()
  // \li BeginStillRender()
  // \li PerformRender()
  // \li EndStillRender() 
  // in that order.
  void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  // Internally calls 
  // \li UpdateAllDisplays()
  // \li BeginInteractiveRender()
  // \li PerformRender()
  // \li EndInteractiveRender() 
  // in that order.
  void InteractiveRender();
 
  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested. A view subclass can define a set of strategies that are
  // registered with it which the representation can ask. A representation
  // strategy encapsulates the pipelines for managing
  // level-of-detail/parallelism. Views that do not support parallelism or LOD
  // may not provide any strategy at all.
  // Subclasses should override NewStrategyInternal.
  vtkSMRepresentationStrategy* NewStrategy(int dataType);

  // Description:
  // Multi-view methods:
  // This is useful when using multiple views. Set the dimensions
  // of the GUI with all the multiple views take together.
  vtkSetVector2Macro(GUISize, int);
  vtkGetVector2Macro(GUISize, int);

  // Description:
  // Multi-view methods:
  // This is useful when using multiple views. 
  // Sets the position of the view associated with this module inside
  // the server render window. (0,0) corresponds to upper left corner.
  vtkSetVector2Macro(ViewPosition, int);
  vtkGetVector2Macro(ViewPosition, int);

  // Description:
  // Returns the memory size for the visible data.
  unsigned long GetVisibleDisplayedDataSize();

  // Description:
  // Returns the full resoultion memory size for the all the visible
  // representations irresepective of whether low resolution (LOD) data is
  // currently shown in the view.
  unsigned long GetVisibileFullResDataSize();
 
  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*);

  // Description:
  // Every view has a update time i.e. a time to which all the representations
  // in it will be updated to (ofcourse, every representation has flags on it
  // to avoid using the view's update time).
  void SetViewUpdateTime(double time);
  vtkGetMacro(ViewUpdateTime, double);

  // Description:
  // Get/Set the cache limit (in kilobytes) for each process. If cache size
  // grows beyond the limit, no caching is done on any of the processes.
  vtkGetMacro(CacheLimit, int);
  vtkSetMacro(CacheLimit, int);

  // Description:
  // When caching is enabled (typically, when playing animations,
  // this time must be updated when each frame is changed.
  void SetCacheTime(double time);
  vtkGetMacro(CacheTime, double);

  // Description:
  // Set/get whether cached geometry should be used whenever possible.
  void SetUseCache(int);
  vtkGetMacro(UseCache, int);

//BTX
protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy();

  // Description:
  // Method called at the start of StillRender().
  // Before this method is called, we as assured that all representations are
  // updated. However, if this method invalidates any of the representations,
  // it must set ForceRepresentationUpdate flag to true so that representations
  // are updated once again before performing the render.
  virtual void BeginStillRender();

  // Description:
  // Method called at the end of StillRender().
  virtual void EndStillRender();

  // Description:
  // Method called at the start of InteractiveRender().
  // Before this method is called, we as assured that all representations are
  // updated. However, if this method invalidates any of the representations,
  virtual void BeginInteractiveRender();
  
  // Description:
  // Method called at the end of InteractiveRender().
  virtual void EndInteractiveRender();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender(). 
  // Default implementation is empty.
  virtual void PerformRender() {};

  // Description:
  // Called at the start of CreateVTKObjects().
  // If returns false, CreateVTKObjects is aborted.
  virtual bool BeginCreateVTKObjects() {return true; }

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void EndCreateVTKObjects() {};

  // Description:
  // Called by AddRepresentation(). Subclasses can override to add 
  // observers etc.
  virtual void AddRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Called by RemoveRepresentation(). Subclasses can override to remove 
  // observers etc.
  virtual void RemoveRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Called to process events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);

  // Description:
  // Returns the observer that the subclasses can use to listen to additional
  // events. Additionally these subclasses should override
  // ProcessEvents() to handle these events.
  vtkCommand* GetObserver();

  // Description:
  // Called to create the vtk objects.
  virtual void CreateVTKObjects();

  // Description:
  // Implementation to create a representation requested strategy.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int vtkNotUsed(dataType))
    { return 0; }

  // Description:
  // Equivaluent to 
  // vtkSMProxyProperty::SafeDownCast(consumer)->GetProperty(
  //    propertyname)->AddProxy(producer).
  void Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname="Input");

  // Description:
  // Marks all data size information as invalid.
  void InvalidateDataSizes();

  // Description:
  // In multiview setups, some viewmodules may share certain objects with each
  // other. This method is used in such cases to give such views an opportunity
  // to share those objects.
  // Default implementation is empty.
  virtual void InitializeForMultiView(vtkSMViewProxy* vtkNotUsed(otherView)) {}

  // Collection of representation objects added to this view.
  vtkCollection* Representations;

  // Information object used to keep rendering specific information.
  // It is passed to the representations added to the view.
  vtkInformation* Information;

  int GUISize[2];
  int ViewPosition[2];

  // Can be set to true in BeginInteractiveRender() or BeginStillRender() is the
  // representations are modified by these methods. This flag is reset at the
  // end of the render.
  void SetForceRepresentationUpdate(bool b)
    { this->ForceRepresentationUpdate = b; }
  vtkGetMacro(ForceRepresentationUpdate, bool);

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  vtkSetStringMacro(DefaultRepresentationName);
  char* DefaultRepresentationName;
private:
  vtkSMViewProxy(const vtkSMViewProxy&); // Not implemented
  void operator=(const vtkSMViewProxy&); // Not implemented

  class Command;
  friend class Command;
  Command* Observer;

  // Whenever user create multiple view modules on the same server connection,
  // for each of the views to work, it may be necessary to share certain server
  // side objects. This has to happen before CreateVTKObjects() irrespective of
  // whether the views are registered with the proxy manager or not. For that
  // purpose we use the MultiViewInitializer. 
  // Before calling BeginCreateVTKObjects(), vtkSMViewProxy asks the
  // MultiViewInitializer for any other view that may already have been created
  // and then calls InitializeForMultiView(). InitializeForMultiView()
  // implementations should share the all necessary objects. On successful proxy
  // creation, the proxy is added to the list maintained by
  // MultiViewInitializer. When the proxy is destroyed, it removes itself from
  // this list.
  // MultiViewInitializer does not use reference counting. MultiViewInitializer
  // singleton is created when the view proxy is created, if not already
  // present, and destroyed after the last view proxy in the list maintained by
  // it is destroyed.
  class vtkMultiViewInitializer;
  friend class vtkMultiViewInitializer;
  static vtkMultiViewInitializer* MultiViewInitializer;
  static vtkMultiViewInitializer* GetMultiViewInitializer();
  static void CleanMultiViewInitializer();

  unsigned long DisplayedDataSize;
  bool DisplayedDataSizeValid;

  unsigned long FullResDataSize;
  bool FullResDataSizeValid;

  bool ForceRepresentationUpdate;

  double ViewUpdateTime;
  bool ViewUpdateTimeInitialized;

  int CacheLimit; // in KiloBytes.
  double CacheTime;
  int UseCache;
//ETX
};

#endif

