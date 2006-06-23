/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewProxy.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

vtkCxxRevisionMacro(pqRenderViewProxy, "1.8");
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
  if (this->RenderModule)
    {
    // EventuallyRender is never called during interactive render,
    // this should try to collapse render requests and all,
    // for now simply StillRender.
    this->RenderModule->getRenderModuleProxy()->StillRender();
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewProxy::Render()
{
  if (this->RenderModule)
    {
    // pqRenderViewProxy::Render() is only called for interactive render.
    // Hence...
    // render LOD's
    this->RenderModule->getRenderModuleProxy()->InteractiveRender();
    }
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
  os << indent << "RenderModule: " << this->RenderModule << endl;
}
