/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDataRepresentationProxy - Superclass for representations that
// have some data input.
// .SECTION Description
// vtkSMDataRepresentationProxy is a superclass for representations that
// consume data i.e. require some input.
//
// A data representation takes on selection obligations. This makes it possible
// for representations to show selections on the input data. If a subclass does
// not support selection then \c SelectionSupported flag should be false.
// If selection is supported, the representation should be able to "render" a
// proxy for a vtkSelection in the view. By default \c SelectionSupported is
// false.
//
// When dealing with data inputs, often times some data pipeline is needed to
// ensure that the data is made available to the location where the rendering
// happens. Also there might be some data distribution obligations that such
// representations have to fulfill. These depend on the type of view that the
// representation gets added to. These data-distribution/delivery pipelines are
// abstracted in what we call \c {Representation Strategies}.
//
// When a representation is added to a view, this class class
// InitializeStrategy() which gives subclasses an opportunity to get different
// types of strategies from the view and set them up it their data pipelines.
//
// Subclasses are free to not use any strategies at all. In which case they have
// to provide implementations for Update(), UpdateRequired(),
// MarkDirty(), SetUpdateTimeInternal(), GetRepresentedDataInformation(),
// GetFullResMemorySize(), GetDisplayedMemorySize(), GetLODMemorySize().
// This class provides default implementation for these methods for
// representations using a collection of strategies. If these startegies are
// used conditionally, then the subclass must override the above mentioned
// methods are provide its own implementations.
//
// .SECTION Caveats
// \li Generally speaking, this proxy requires that the input is set before the
// representation is added to any view. This requirement stems from the fact
// that to deterine the right strategy to use for a representation, it may be
// necessary to know the data type of the input data.
// .SECTION See Also
// vtkSMRepresentationStrategy

#ifndef __vtkSMDataRepresentationProxy_h
#define __vtkSMDataRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSelection;
class vtkSMDataRepresentationProxyObserver;
class vtkSMPropertyLink;
class vtkSMRepresentationStrategy;
class vtkSMRepresentationStrategyVector;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMDataRepresentationProxy :
  public vtkSMRepresentationProxy
{
public:
  vtkTypeMacro(vtkSMDataRepresentationProxy,
    vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkSMInputProperty requires that the consumer proxy support
  // AddInput() method. Hence, this method is defined. This method
  // sets up the input connection.
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
  // Get the information about the data shown by this representation.
  // Some representations use some pre-processing before displaying the data eg.
  // apply a geometry filter. This is the data information after that
  // pre-processing stage. If \c update is set to false, the pipeline is not
  // updated before gathering the information, (\c update is true by default).
  // Default implementation simply returns the data information from the input.
  // When update is true, the pipeline until the filter from which the
  // information is obtained is updated. This is preferred to calling Update()
  // on the representation directly, since an Update include delivering of the
  // data to the destination where it will be rendered.
  virtual vtkPVDataInformation* GetRepresentedDataInformation(bool update=true);

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
  // Set the time used during update requests.
  // Default implementation passes the time to the strategy, if any. If
  // subclasses don't use any stratgy, they may want to override this method.
  // The representation should respect the update time set only when
  // UseViewUpdateTime is false. If UseViewUpdateTime is true, then the
  // representation's UpdateTime is ignored, instead the ViewUpdateTime passed
  // on by the view is used.
  virtual void SetUpdateTime(double time);

  // Description:
  // When set to true, the view's ViewUpdateTime is used to determine the update
  // time for the representation (default behaviour). If set to false,
  // UpdateTime (set using SetUpdateTime()) is used.
  // Default implementation passes the status of this flag to all strategies.
  void SetUseViewUpdateTime(bool);
  vtkGetMacro(UseViewUpdateTime, bool);

  // Description:
  // Fill the activeStrategies collection with strategies that are currently
  // active i.e. being used.
//BTX
  virtual void GetActiveStrategies(
    vtkSMRepresentationStrategyVector& activeStrategies);
//ETX
  // Description:
  // Views typically support a mechanism to create a selection in the view
  // itself, eg. by click-and-dragging over a region in the view. The view
  // passes this selection to each of the representations and asks them to
  // convert it to a proxy for a selection which can be set on the view.
  // It a representation does not support selection creation, it should simply
  // return NULL.  On success, this method returns a new vtkSMProxy instance
  // which the caller must free after use.
  virtual vtkSMProxy* ConvertSelection(vtkSelection* vtkNotUsed(input))
    { return 0; }

  // Description:
  // Representations can request ordered compositing eg. representations that
  // perform volume rendering or have opacity < 1.0. Such representations must
  // return true for this method. Default implementation return false.
  virtual bool GetOrderedCompositingNeeded()
    { return false; }

//BTX

  // Description:
  // Called when a representation is added to a view.
  // Returns true on success.
  // Added to call InitializeStrategy() to give subclassess the opportunity to
  // set up pipelines involving compositing strategy it they support it.
  virtual bool AddToView(vtkSMViewProxy* view);

  enum AttributeTypes
    {
    POINT_DATA =0,
    CELL_DATA  =1,
    FIELD_DATA =2
    };

  // Description:
  // Called by the view to pass the view's update time to the representation.
  virtual void SetViewUpdateTime(double time);

  // Description:
  // HACK: vtkSMAnimationSceneGeometryWriter needs acces to the processed data
  // so save out. This method should return the proxy that goes in as the input
  // to strategies (eg. in case of SurfaceRepresentation, it is the geometry
  // filter).
  virtual vtkSMProxy* GetProcessedConsumer()
    { return 0; }

  // Description:
  // Returns the data size of the display data. When using LOD this is the
  // low-res data size, else it's same as GetFullResMemorySize().
  // This may trigger a pipeline update to obtain correct data sizes.
  virtual unsigned long GetDisplayedMemorySize();

  // Description:
  // Returns the data size for the full-res data.
  // This may trigger a pipeline update to obtain correct data sizes.
  virtual unsigned long GetFullResMemorySize();

  // Description:
  // Overridden to make the Strategy modified as well.
  // The strategy is not marked modified if the modifiedProxy == this,
  // thus if the changes to representation itself invalidates the data pipelines
  // it must explicity mark the strategy invalid.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

protected:
  vtkSMDataRepresentationProxy();
  ~vtkSMDataRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // If this method returns false, CreateVTKObjects() is aborted.
  // Overridden to abort CreateVTKObjects() only if the input has
  // been initialized correctly.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects().
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  // Overridden to setup view time link.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Some representations may require lod/compositing strategies from the view
  // proxy. This method gives such subclasses an opportunity to as the view
  // module for the right kind of strategy and plug it in the representation
  // pipeline. Returns true on success. Default implementation suffices for
  // representation that don't use strategies.
  virtual bool InitializeStrategy(vtkSMViewProxy* vtkNotUsed(view))
    { return true; }

  // Description:
  // Set the representation strategy. Simply initializes the Strategy ivar and
  // initializes UpdateTime.
  void AddStrategy(vtkSMRepresentationStrategy*);

  // Description:
  // Provide access to Input for subclasses.
  vtkGetObjectMacro(InputProxy, vtkSMSourceProxy);

  // Description:
  // Subclasses can use this method to traverse up the input connection
  // from this representation and mark them modified.
  void MarkUpstreamModified();

  // Description:
  // Returns the observer.
  vtkCommand* GetObserver();

  // Description:
  // Pass the actual update time to use to all strategies.
  // When not using strategies, make sure that this method is overridden to pass
  // the update time correctly.
  virtual void SetUpdateTimeInternal(double time);

  // These are the representation strategies used for data display.
  vtkSMRepresentationStrategyVector* RepresentationStrategies;

  double UpdateTime;
  bool UpdateTimeInitialized;

  bool UseViewUpdateTime;

  unsigned int OutputPort;

private:
  vtkSMDataRepresentationProxy(const vtkSMDataRepresentationProxy&); // Not implemented
  void operator=(const vtkSMDataRepresentationProxy&); // Not implemented

  void SetInputProxy(vtkSMSourceProxy*);
  vtkSMSourceProxy* InputProxy;

  vtkSMDataRepresentationProxyObserver* Observer;
//ETX
};

#endif

