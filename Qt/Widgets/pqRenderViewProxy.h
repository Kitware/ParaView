/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqRenderViewProxy_h
#define _pqRenderViewProxy_h

#include "QtWidgetsExport.h"
#include "vtkPVRenderViewProxy.h"

class vtkSMRenderModuleProxy;

/// Integrates the PVS render window with the Qt window
class QTWIDGETS_EXPORT pqRenderViewProxy : public vtkPVRenderViewProxy
{
public:
  static pqRenderViewProxy* New();
  vtkTypeRevisionMacro(pqRenderViewProxy,vtkPVRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  virtual void Render();
  virtual void EventuallyRender();
  
  vtkRenderWindow* GetRenderWindow();
  void SetRenderModule(vtkSMRenderModuleProxy* rm);
 
protected:
  pqRenderViewProxy();
  ~pqRenderViewProxy();
 
private:
  pqRenderViewProxy(const pqRenderViewProxy&); // Not implemented
  void operator=(const pqRenderViewProxy&); // Not implemented

  vtkSMRenderModuleProxy* RenderModule;
};

#endif

