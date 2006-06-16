/*=========================================================================

   Program:   ParaQ
   Module:    pqRenderViewProxy.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqRenderViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMRenderModuleProxy.h"

#include "pqRenderModule.h"

vtkCxxRevisionMacro(pqRenderViewProxy, "1.6");
vtkStandardNewMacro(pqRenderViewProxy);
//-----------------------------------------------------------------------------
pqRenderViewProxy::pqRenderViewProxy()
{
  this->RenderModule = 0;
}

//-----------------------------------------------------------------------------
pqRenderViewProxy::~pqRenderViewProxy()
{
  this->RenderModule = 0;
}

//-----------------------------------------------------------------------------
void pqRenderViewProxy::EventuallyRender()
{
  this->Render();
}

//-----------------------------------------------------------------------------
void pqRenderViewProxy::Render()
{
  if (!this->RenderModule)
    {
    return;
    }
  // render LOD's
  //this->RenderModule->getRenderModuleProxy()->InteractiveRender();

  // do not render LOD's
  this->RenderModule->getRenderModuleProxy()->StillRender();
}

//-----------------------------------------------------------------------------
vtkRenderWindow* pqRenderViewProxy::GetRenderWindow()
{
  if (!this->RenderModule)
    {
    return 0;
    }
  return this->RenderModule->getRenderModuleProxy()->GetRenderWindow();
}

//-----------------------------------------------------------------------------
void pqRenderViewProxy::setRenderModule(pqRenderModule* rm)
{
  this->RenderModule = rm;
}

//-----------------------------------------------------------------------------
void pqRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
