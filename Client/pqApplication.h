// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME pqApplication
//
// .SECTION Description
//
// pqApplication simplifies some of the tasks of making ParaView
// targeted applications.
//

#ifndef _pqApplication_h
#define _pqApplication_h

#include <vtkObject.h>

class vtkProcessModule;
class vtkRenderWindow;
class vtkSMDisplayProxy;
class vtkSMProxyManager;
class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkPVGenericRenderWindowInteractor;

class pqApplication : public vtkObject
{
public:
  vtkTypeRevisionMacro(pqApplication, vtkObject);
  static pqApplication *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get the process module.
  vtkGetObjectMacro(ProcessModule, vtkProcessModule);

  // Description:
  // Get the SMApplication object.
//  vtkGetObjectMacro(SMApplication, vtkSMApplication);

  // Description:
  // Initializes the operations of this class.  Call this method before any
  // other method.  This method does <I>not</I> setup the render module.
  // Call SetupRenderModule after calling Initialize.
  virtual void Initialize(vtkProcessModule *);

  // Description:
  // Finalizes this class.
  virtual void Finalize();

  // Description:
  // Create a render module of the given name.  If a name is not given,
  // then the render module first sees if the user selected a render module
  // in the command line arguments.  Failing that, the best known render
  // module is picked based on the server capabilities.  Indirectly calls
  // SetRenderModuleProxy.  Returns 1 on success and 0 for failure.
  virtual int SetupRenderModule(const char *renderModuleName = NULL);

  // Description:
  // Gets the vtkSMRenderModuleProxy used to manage the rendering of the
  // render window.
  virtual void SetRenderModuleProxy(vtkSMRenderModuleProxy *rm);
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);

  // Description:
  // Returns a vtkSMProxyManager.  This can be used to safely build
  // pipelines regardless of the mode the application is running in
  // (standalone, client/server, client/server/render server).
  virtual vtkSMProxyManager *GetProxyManager();

  // Description:
  // Adds a part to be displayed.  The part should be a source or filter
  // created with the proxy manager (retrived with GetProxyManager).  This
  // method returns a vtkSMDisplayProxy, which can be used to modify how
  // the part is displayed or to remove the part with RemovePart.  The
  // vtkSMDisplayProxy is maintained internally, so the calling application
  // does NOT have to delete it (it can be ignored).
  virtual vtkSMDisplayProxy *AddPart(vtkSMSourceProxy *part);

  // Description:
  // Remove a part created with AddPart.
  virtual void RemovePart(vtkSMDisplayProxy *part);

  // Description:
  // Reset the camera based on the parts added.
  virtual void ResetCamera();

  // Description:
  // Performs a still render.  This is generally a slow but detailed
  // rendering.  The method of rendering is determined by the render
  // module.
  virtual void StillRender();

  // Description:
  // Performs an interactive render.  This is generally fast but of low
  // detail.  Use this during iteraction when fast frame rates are more
  // important than detail.  The method of rendering is determined by the
  // render module.
  virtual void InteractiveRender();

/*
  // Description:
  // Creates a vtkPVGenericRenderWindowInteractor, a
  // vtksnlPVInteractorStyle, and some vtksnlPVCameraManipulators and
  // attaches them to the render window.  Note that the interactor does not
  // have an event handler (Start has an empty implementation).  For any
  // interesting app, this should not matter since the GUI will have its
  // own event handler that the interactor will plug into.
  virtual void EstablishInteractor();
*/

  // Description:
  // Returns the interactor created with EstablishInteractor.
  virtual vtkPVGenericRenderWindowInteractor *GetInteractor();

protected:
  pqApplication();
  virtual ~pqApplication();

  vtkProcessModule *ProcessModule;
  vtkSMRenderModuleProxy *RenderModuleProxy;

private:
  pqApplication(const pqApplication &);   // Not implemented.
  void operator=(const pqApplication &);         // Not implemented.
};

#endif //_pqApplication_h
