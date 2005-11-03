/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqRenderViewProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMRenderModuleProxy.h"

vtkCxxRevisionMacro(pqRenderViewProxy, "1.1");
vtkStandardNewMacro(pqRenderViewProxy);

pqRenderViewProxy::pqRenderViewProxy()
{
  this->RenderModule = 0;
}

pqRenderViewProxy::~pqRenderViewProxy()
{
  this->RenderModule = 0;
}

void pqRenderViewProxy::EventuallyRender()
{
  this->Render();
}

void pqRenderViewProxy::Render()
{
  // render LOD's
  //RenderModule->InteractiveRender();

  // do not render LOD's
  this->RenderModule->StillRender();
}

vtkRenderWindow* pqRenderViewProxy::GetRenderWindow()
{
  if (!this->RenderModule)
    {
    return 0;
    }
  return this->RenderModule->GetRenderWindow();
}

void pqRenderViewProxy::SetRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->RenderModule = rm;
}

void pqRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
