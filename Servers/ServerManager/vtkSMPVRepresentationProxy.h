/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPVRepresentationProxy - a composite representation proxy suitable
// for showing data in ParaView.
// .SECTION Description
// vtkSMPVRepresentationProxy combines surface representation and volume
// representation proxies typically used for displaying data.
// This class also takes over the selection obligations for all the internal
// representations, i.e. is disables showing of selection in all the internal
// representations, and manages it. This avoids duplicate execution of extract
// selection filter for each of the internal representations.

#ifndef __vtkSMPVRepresentationProxy_h
#define __vtkSMPVRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMPVRepresentationProxy : 
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMPVRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMPVRepresentationProxy, vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the type of representation. 
  void SetRepresentation(int type);
  vtkGetMacro(Representation, int);

  // Description:
  // Set the representation's visibility.
  virtual void SetVisibility(int visible);

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
  virtual void SetUseViewUpdateTime(bool);

  // Description:
  // Called by the view to pass the view's update time to the representation.
  virtual void SetViewUpdateTime(double time);

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
  virtual vtkSMProxy* ConvertSelection(vtkSelection* input);

  // Description:
  // Representations can request ordered compositing eg. representations that
  // perform volume rendering or have opacity < 1.0. Such representations must
  // return true for this method. Default implementation return false.
  virtual bool GetOrderedCompositingNeeded();

  // Description:
  // Overridden to pass the view information to all the internal
  // representations.
  virtual void SetViewInformation(vtkInformation*);

  // Description:
  // HACK: vtkSMAnimationSceneGeometryWriter needs acces to the processed data
  // so save out. This method should return the proxy that goes in as the input
  // to strategies (eg. in case of SurfaceRepresentation, it is the geometry
  // filter).
  virtual vtkSMProxy* GetProcessedConsumer();

//BTX
  enum RepresentationType
    {
    POINTS=0,
    WIREFRAME=1,
    SURFACE=2,
    OUTLINE=3,
    VOLUME=4
    };
protected:
  vtkSMPVRepresentationProxy();
  ~vtkSMPVRepresentationProxy();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view); 

  vtkSMDataRepresentationProxy* SurfaceRepresentation;
  vtkSMDataRepresentationProxy* VolumeRepresentation;
  vtkSMDataRepresentationProxy* OutlineRepresentation;
  vtkSMDataRepresentationProxy* ActiveRepresentation;

  int Representation;
  int SelectionVisibility;

private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPVRepresentationProxy&); // Not implemented
//ETX
};

#endif

