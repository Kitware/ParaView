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
// rendering work at the GUI level. The VTKObject corresponding to this class
// has to be a vtkView subclass.
// .SECTION Events
// \li vtkCommand::StartEvent(callData: int:0) -- start of StillRender().
// \li vtkCommand::EndEvent(callData: int:0) -- end of StillRender().
// \li vtkCommand::StartEvent(callData: int:1) -- start of InteractiveRender().
// \li vtkCommand::EndEvent(callData: int:1) -- end of InteractiveRender().

#ifndef __vtkSMViewProxy_h
#define __vtkSMViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkImageData;
class vtkRenderWindow;
class vtkSMRepresentationProxy;
class vtkSMSourceProxy;
class vtkView;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/Disable a view.
  vtkSetMacro(Enable, bool);
  vtkGetMacro(Enable, bool);
  vtkBooleanMacro(Enable, bool);

  // Description:
  // Renders the view using full resolution.
  virtual void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  virtual void InteractiveRender();

  // Description:
  // Called vtkPVView::Update on the server-side.
  virtual void Update();

  // Description:
  // Returns true if the view can display the data produced by the producer's
  // port. Internally calls GetRepresentationType() and returns true only if the
  // type is valid a representation proxy of that type can be created.
  virtual bool CanDisplayData(vtkSMSourceProxy* producer, int outputPort);

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  // In version 4.1 and earlier, subclasses overrode this method. Since 4.2, the
  // preferred way is to simply override GetRepresentationType(). That
  // ensures that CreateDefaultRepresentation() and CanDisplayData() both
  // work as expected.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int);

  // Description:
  // Returns the xml name of the representation proxy to create to show the data
  // produced in this view, if any. Default implementation checks if the
  // producer has any "Hints" that define the representation to create in this
  // view and if so, returns that.
  // Or if this->DefaultRepresentationName is set and its Input property
  // can accept the data produced, returns this->DefaultRepresentationName.
  // Subclasses should override this method.
  virtual const char* GetRepresentationType(
    vtkSMSourceProxy* producer, int outputPort);

  // Description:
  // Finds the representation proxy showing the data produced by the provided
  // producer, if any. Note the representation may not necessarily be visible.
  virtual vtkSMRepresentationProxy* FindRepresentation(vtkSMSourceProxy* producer, int outputPort);

  // Description:
  // Captures a image from this view. Default implementation returns NULL.
  // Subclasses should override CaptureWindowInternal() to do the actual image
  // capture.
  vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Returns the client-side vtkView, if any.
  vtkView* GetClientSideView();

  // Description:
  // Saves a screenshot of the view to disk. The writerName argument specifies
  // the vtkImageWriter subclass to use.
  int WriteImage(const char* filename, const char* writerName, int magnification=1);

  // Description:
  // Return true any internal representation is dirty. This can be usefull to
  // know if the internal geometry has changed.
  // DEPRECATED: Use GetNeedsUpdate() instead.
  virtual bool HasDirtyRepresentation() { return this->GetNeedsUpdate(); }

  // Description:
  // Returns true if the subsequent call to Update() will result in an actual
  // update. If returned true, it means that the view thinks its rendering is
  // obsolete and needs to be re-generated.
  vtkGetMacro(NeedsUpdate, bool);

  // Description:
  // Return the vtkRenderWindow used by this view, if any. Note, views like
  // vtkSMComparativeViewProxy can have more than 1 render window in play, in
  // which case, using this method alone may yield incorrect results. Also,
  // certain views don't use a vtkRenderWindow at all (e.g. Spreadsheet View),
  // in which case, this method will return NULL. Default implementation returns
  // NULL.
  virtual vtkRenderWindow* GetRenderWindow() { return NULL; }

//BTX
protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy();

  //
  // Description:
  // Subclasses should override this method to do the actual image capture.
  virtual vtkImageData* CaptureWindowInternal(int vtkNotUsed(magnification))
    { return NULL; }

  virtual vtkTypeUInt32 PreRender(bool vtkNotUsed(interactive))
    { return this->GetLocation(); }
  virtual void PostRender(bool vtkNotUsed(interactive)) {}

  // Description:
  // Subclasses should override this method and return false if the rendering
  // context is not ready for rendering at this moment. This method is called in
  // StillRender() and InteractiveRender() calls before the actual render to
  // ensure that we don't attempt to render when the rendering context is not
  // ready.
  // Default implementation uses this->GetRenderWindow() and checks if that
  // window is drawable.
  virtual bool IsContextReadyForRendering();

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void CreateVTKObjects();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);

  vtkSetStringMacro(DefaultRepresentationName);
  char* DefaultRepresentationName;

  bool Enable;

private:
  vtkSMViewProxy(const vtkSMViewProxy&); // Not implemented
  void operator=(const vtkSMViewProxy&); // Not implemented

  // When view's time changes, there's no way for the client-side proxies to
  // know that they may re-execute and their data info is invalid. So mark those
  // dirty explicitly.
  void ViewTimeChanged();
//ETX
};

#endif
