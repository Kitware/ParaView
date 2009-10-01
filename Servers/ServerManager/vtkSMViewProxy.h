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
  virtual void RemoveAllRepresentations();

  // Description:
  // Updates the data pipelines for all visible representations.
  virtual void UpdateAllRepresentations();

  // Description:
  // Renders the view using full resolution.
  // Internally calls 
  // \li BeginStillRender()
  // \li UpdateAllRepresentations()
  // \li PerformRender()
  // \li EndStillRender() 
  // in that order.
  virtual void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  // Internally calls 
  // \li BeginInteractiveRender()
  // \li UpdateAllRepresentations()
  // \li PerformRender()
  // \li EndInteractiveRender() 
  // in that order.
  virtual void InteractiveRender();
 
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
  // Create a default representation for the given output port of source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int opport);
  vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy* proxy)
    { return this->CreateDefaultRepresentation(proxy, 0); }


  // Description:
  // Every view has a update time i.e. a time to which all the representations
  // in it will be updated to (ofcourse, every representation has flags on it
  // to avoid using the view's update time).
  virtual void SetViewUpdateTime(double time);
  vtkGetMacro(ViewUpdateTime, double);

  // Description:
  // When caching is enabled (typically, when playing animations,
  // this time must be updated when each frame is changed.
  virtual void SetCacheTime(double time);
  vtkGetMacro(CacheTime, double);

  // Description:
  // Set/get whether cached geometry should be used whenever possible.
  // Called by vtkSMAnimationSceneProxy to enable caching.
  virtual void SetUseCache(int);
  vtkGetMacro(UseCache, int);

  // Description:
  // Generally each view type is different class of view eg. bar char view, line
  // plot view etc. However in some cases a different view types are indeed the
  // same class of view the only different being that each one of them works in
  // a different configuration eg. "RenderView" in builin mode, 
  // "IceTDesktopRenderView" in remote render mode etc. This method is used to
  // determine what type of view needs to be created for the given class. When
  // user requests the creation of a view class, the application can call this
  // method on a prototype instantaiated for the requested class and the
  // determine the actual xmlname for the view to create. Default implementation
  // simply returns the XML name for the prototype (which is the case where view
  // types don't change based on configurations).
  virtual const char* GetSuggestedViewType(vtkIdType vtkNotUsed(connectionId))
    {
    return this->GetXMLName();
    }

  // Description:
  // Views keep this false to schedule additional passes.
  virtual int GetDisplayDone() { return 1; }

  // Description:
  // Implementation to create a representation requested strategy. 
  //DDM TODO Do I have to make this public?
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int vtkNotUsed(dataType))
    { return 0; }

  //Description:
  // When set, strategy creation will defer to the otherView.
  // Note this is not reference counted to avoid circular dependency
  void SetNewStrategyHelper(vtkSMViewProxy *otherView)
  {
    this->NewStrategyHelper = otherView;
  }
  vtkGetObjectMacro(NewStrategyHelper, vtkSMViewProxy);

  // Collection of representation objects added to this view.
  vtkCollection* Representations; //DDM TODO Do I have to make this public?

  // Description:
  // Called by AddRepresentation(). Subclasses can override to add 
  // observers etc. //DDM TODO Do I have to make this public?
  virtual void AddRepresentationInternal(vtkSMRepresentationProxy* rep);

  //Description: For streaming provide a hook to stop multipass rendering.
  virtual void Interrupt() {};

//BTX
protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy();

  // Description:
  // Method called at the start of StillRender().
  // All the representations are in an un-updated state. It is not recommended
  // to update the representation until important decisions such as use of
  // lod/compositing are made. It is safe however to use
  // GetVisibileFullResDataSize() or GetVisibleDisplayedDataSize() since these
  // methods only partially update the representation pipelines, if at all.
  virtual void BeginStillRender();

  // Description:
  // Method called at the end of StillRender().
  virtual void EndStillRender();

  // Description:
  // Method called at the start of InteractiveRender().
  // All the representations are in an un-updated state. It is not recommended
  // to update the representation until important decisions such as use of
  // lod/compositing are made. It is safe however to use
  // GetVisibileFullResDataSize() or GetVisibleDisplayedDataSize() since these
  // methods only partially update the representation pipelines, if at all.
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
  // Equivaluent to 
  // vtkSMProxyProperty::SafeDownCast(consumer)->GetProperty(
  //    propertyname)->AddProxy(producer).
  void Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname="Input");

  // Description:
  // Marks all data size information as invalid.
  void InvalidateDataSizes();

  // Description:
  // Returns the memory size for the visible data.
  // If some of the visible representations are dirty, this results in updating
  // those representations partially. i.e. the representation is updated only
  // until the filter from which the data size information is updated (which is
  // generally before the expensive data transfer filters).
  unsigned long GetVisibleDisplayedDataSize();

  // Description:
  // Returns the full resoultion memory size for the all the visible
  // representations irresepective of whether low resolution (LOD) data is
  // currently shown in the view.
  // If some of the visible representations are dirty, this results in updating
  // those representations partially. i.e. the representation is updated only
  // until the filter from which the data size information is updated (which is
  // generally before the expensive data transfer filters).
  unsigned long GetVisibileFullResDataSize();

  // Description:
  // In multiview setups, some viewmodules may share certain objects with each
  // other. This method is used in such cases to give such views an opportunity
  // to share those objects.
  // Default implementation is empty.
  virtual void InitializeForMultiView(vtkSMViewProxy* vtkNotUsed(otherView)) {}

  // Information object used to keep rendering specific information.
  // It is passed to the representations added to the view.
  vtkInformation* Information;

  int GUISize[2];
  int ViewPosition[2];

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  vtkSetStringMacro(DefaultRepresentationName);
  char* DefaultRepresentationName;

  vtkSMViewProxy *NewStrategyHelper;

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

  double ViewUpdateTime;
  bool ViewUpdateTimeInitialized;

  double CacheTime;
  int UseCache;

  bool InRender; // used to avoid render call while a render is in progress.
//ETX
};

#endif

