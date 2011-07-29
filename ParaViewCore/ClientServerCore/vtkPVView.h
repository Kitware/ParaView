/*=========================================================================

  Program:   ParaView
  Module:    vtkPVView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVView - baseclass for all ParaView views.
// .SECTION Description
// vtkPVView adds API to vtkView for ParaView specific views. Typically, one
// writes a simple vtkView subclass for their custom view. Then one subclasses
// vtkPVView to use their own vtkView subclass with added support for
// parallel rendering, tile-displays and client-server. Even if the view is
// client-only view, it needs to address these other configuration gracefully.

#ifndef __vtkPVView_h
#define __vtkPVView_h

#include "vtkView.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkInformation;
class vtkInformationRequestKey;
class vtkInformationVector;
class vtkPVSynchronizedRenderWindows;

class VTK_EXPORT vtkPVView : public vtkView
{
public:
  vtkTypeMacro(vtkPVView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum
    {
    ViewTimeChangedEvent=9000
    };

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

  // Description:
  // Set the position on this view in the multiview configuration.
  // This can be called only after Initialize().
  // @CallOnAllProcessess
  virtual void SetPosition(int, int);

  // Description:
  // Set the size of this view in the multiview configuration.
  // This can be called only after Initialize().
  // @CallOnAllProcessess
  virtual void SetSize(int, int);

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender()=0;

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender()=0;

  // Description:
  // This encapsulates a whole lot of logic for
  // communication between processes. Given the ton of information this class
  // keeps, it can easily aid vtkViews to synchronize information such as bounds/
  // data-size among all processes efficiently. This can be achieved by using
  // these methods.
  // Note that these methods should be called on all processes at the same time
  // otherwise we will have deadlocks.
  // We may make this API generic in future, for now this works.
  bool SynchronizeBounds(double bounds[6]);
  bool SynchronizeSize(double &size);
  bool SynchronizeSize(unsigned int &size);

  // Description:
  // Get/Set the time this view is showing. Whenever time is changed, this fires
  // a ViewTimeChangedEvent event.
  // @CallOnAllProcessess
  virtual void SetViewTime(double value);
  vtkGetMacro(ViewTime, double);

  // Description:
  // Get/Set the cache key. When caching is enabled, this key is used to
  // identify what geometry cache to use for the current render. It is passed on
  // to the representations in vtkPVView::Update(). The CacheKey is respected
  // only when UseCache is true.
  // @CallOnAllProcessess
  vtkSetMacro(CacheKey, double);
  vtkGetMacro(CacheKey, double);

  // Description:
  // Get/Set whether caching is enabled.
  // @CallOnAllProcessess
  vtkSetMacro(UseCache, bool);
  vtkGetMacro(UseCache, bool);

  // Description:
  // These methods are used to setup the view for capturing screen shots.
  virtual void PrepareForScreenshot();
  virtual void CleanupAfterScreenshot();

  // Description:
  // This is a Update-Data pass. All representations are expected to update
  // their inputs and prepare geometries for rendering. All heavy work that has
  // to happen only when input-data changes can be done in this pass.
  // This is the first pass.
  static vtkInformationRequestKey* REQUEST_UPDATE();

  // Description:
  // This is a Request-MetaData pass. This happens only after REQUEST_UPDATE()
  // has happened. In this pass representations typically publish information
  // that may be useful for rendering optimizations such as geometry sizes, etc.
  static vtkInformationRequestKey* REQUEST_INFORMATION();

  // Description:
  // This is a Prepare-for-rendering pass. This happens only after
  // REQUEST_UPDATE() has happened. This is called for every render.
  static vtkInformationRequestKey* REQUEST_PREPARE_FOR_RENDER();

  // Description:
  // This is called to make representations deliver data to the rendering nodes
  // after REQUEST_PREPARE_FOR_RENDER(). Called for every render only on those
  // representations that should deliver data.
  static vtkInformationRequestKey* REQUEST_DELIVERY();

  // Description:
  // This is a render pass. This happens only after
  // REQUEST_PREPARE_FOR_RENDER() (and optionally REQUEST_DELIVERY()) has happened.
  // This is called for every render.
  static vtkInformationRequestKey* REQUEST_RENDER();

  // Description:
  // Overridden to not call Update() directly on the input representations,
  // instead use ProcessViewRequest() for all vtkPVDataRepresentations.
  virtual void Update();

//BTX
  vtkGetMacro(Identifier, unsigned int);

protected:
  vtkPVView();
  ~vtkPVView();

  // Description:
  // Returns true if the application is currently in tile display mode.
  bool InTileDisplayMode();

  // vtkPVSynchronizedRenderWindows is used to ensure that this view participates
  // in tile-display configurations. Even if your view subclass a simple
  // Qt-based client-side view that does not render anything on the
  // tile-display, it needs to be "registered with the
  // vtkPVSynchronizedRenderWindows so that the layout on the tile-displays for
  // other views shows up correctly. Ideally you'd want to paste some image on
  // the tile-display, maybe just a capture of the image rendering on the
  // client.
  // If your view needs a vtkRenderWindow, don't directly create it, always get
  // using vtkPVSynchronizedRenderWindows::NewRenderWindow().
  vtkPVSynchronizedRenderWindows* SynchronizedWindows;

  // Description:
  // Every view gets a unique identifier that it uses to register itself with
  // the SynchronizedWindows. This is set in Initialize().
  unsigned int Identifier;

  // Description:
  // These are passed as arguments to
  // vtkDataRepresentation::ProcessViewRequest(). This avoid repeated creation
  // and deletion of vtkInformation objects.
  vtkInformation* RequestInformation;
  vtkInformationVector* ReplyInformationVector;

  // Description:
  // Subclasses can use this method to trigger a pass on all representations.
  void CallProcessViewRequest(
    vtkInformationRequestKey* passType,
    vtkInformation* request, vtkInformationVector* reply);
  double ViewTime;

  double CacheKey;
  bool UseCache;

  int Size[2];
  int Position[2];

private:
  vtkPVView(const vtkPVView&); // Not implemented
  void operator=(const vtkPVView&); // Not implemented

  static vtkWeakPointer<vtkPVSynchronizedRenderWindows>
    SingletonSynchronizedWindows;

  bool ViewTimeValid;
  bool LastRenderOneViewAtATime;
//ETX
};

#endif

