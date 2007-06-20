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
// GetDisplayedDataInformation(), GetFullResDataInformation(), MarkModified(). 
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
  vtkTypeRevisionMacro(vtkSMDataRepresentationProxy, 
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
  // Get the data information for the represented data.
  // Representations that do not have significatant data representations such as
  // 3D widgets, text annotations may return NULL.
  // Overridden to return the strategy's data information. Currently, it returns
  // the data information from the first representation strategy,
  // subclasses using multiple strategies may want to override this.
  virtual vtkPVDataInformation* GetDisplayedDataInformation();

  // Description:
  // Get the data information for the full resolution data irrespective of
  // whether current rendering decision was to use LOD. For representations that
  // don't have separate LOD pipelines, this simply calls
  // GetDisplayedDataInformation().
  // Overridden to return the strategy's data information. Currently, it returns
  // the data information from the first representation strategy,
  // subclasses using multiple strategies may want to override this.
  virtual vtkPVDataInformation* GetFullResDataInformation();

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
  virtual void SetUpdateTime(double time);

  // Description:
  // When set to true, the UpdateTime for this representation is linked to the
  // ViewTime for the view to which this representation is added (default
  // behaviour). Otherwise the update time is independent of the ViewTime.
  void SetUseViewTimeForUpdate(bool);
  vtkGetMacro(UseViewTimeForUpdate, bool);

  // Description:
  // Overridden to make the Strategy modified as well.
  // The strategy is not marked modified if the modifiedProxy == this, 
  // thus if the changes to representation itself invalidates the data pipelines
  // it must explicity mark the strategy invalid.
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

  // Description:
  // Fill the activeStrategies collection with strategies that are currently
  // active i.e. being used.
  virtual void GetActiveStrategies(
    vtkSMRepresentationStrategyVector& activeStrategies);

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

  // These are the representation strategies used for data display. 
  vtkSMRepresentationStrategyVector* RepresentationStrategies;

  double UpdateTime;
  bool UpdateTimeInitialized;
  bool UseViewTimeForUpdate;

  vtkSMPropertyLink* ViewTimeLink;

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

