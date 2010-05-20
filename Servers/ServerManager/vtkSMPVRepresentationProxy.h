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
  vtkTypeMacro(vtkSMPVRepresentationProxy, vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the type of representation.
  virtual void SetRepresentation(int type);
  vtkGetMacro(Representation, int);

  // Description:
  // Set the type of representation for the backface (assuming surfaces are
  // rendered for the front face).
  virtual void SetBackfaceRepresentation(int type);
  vtkGetMacro(BackfaceRepresentation, int);

  // Description:
  // Set the representation's visibility.
  virtual void SetVisibility(int visible);

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

  // Description:
  // Check if this representation has the prop by checking its vtkClientServerID
  virtual bool HasVisibleProp3D(vtkProp3D* prop);

  // Description:
  // Set cube axes visibility. This flag is considered only if
  // this->GetVisibility() == true, otherwise, cube axes is not shown.
  virtual void SetCubeAxesVisibility(int);
  vtkGetMacro(CubeAxesVisibility, int);

  // Description:
  // Get the bounds and transform according to rotation, translation, and scaling.
  // Returns true if the bounds are "valid" (and false otherwise)
  virtual bool GetBounds(double bounds[6]);

  // Description:
  // Overridden to make the Strategy modified as well.
  // The strategy is not marked dirty if the modifiedProxy == this,
  // thus if the changes to representation itself invalidates the data pipelines
  // it must explicity mark the strategy invalid.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // Overridden to setup the "Representation" property's value correctly since
  // it changes based on plugins loaded at run-time.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

//BTX
  enum RepresentationType
    {
    POINTS=0,
    WIREFRAME=1,
    SURFACE=2,
    OUTLINE=3,
    VOLUME=4,
    SURFACE_WITH_EDGES=5,
    SLICE=6,
    USER_DEFINED=100,
    // Special identifiers for back faces.
    FOLLOW_FRONTFACE=400,
    CULL_BACKFACE=401,
    CULL_FRONTFACE=402
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

  // Description:
  // Read attributes from an XML element.
  virtual int CreateSubProxiesAndProperties(vtkSMProxyManager* pm, 
    vtkPVXMLElement *element);

  // Description:
  // Returns true if the active representation is of a surface type.
  virtual bool ActiveRepresentationIsSurface();

  vtkSMDataRepresentationProxy* ActiveRepresentation;
  vtkSMDataRepresentationProxy* BackfaceSurfaceRepresentation;
  vtkSMDataRepresentationProxy* CubeAxesRepresentation;

  int Representation;
  int BackfaceRepresentation;
  int CubeAxesVisibility;
private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPVRepresentationProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

