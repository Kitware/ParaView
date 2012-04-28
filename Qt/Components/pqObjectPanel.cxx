/*=========================================================================

   Program: ParaView
   Module:    pqObjectPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

// this include
#include "pqObjectPanel.h"

// ParaView includes
#include "pqProxy.h"
#include "pqObjectPanelPropertyWidget.h"

//-----------------------------------------------------------------------------
pqObjectPanel::pqObjectPanel(pqProxy* object_proxy, QWidget* p) :
  pqProxyPanel(object_proxy->getProxy(), p), ReferenceProxy(object_proxy)
{
}

//-----------------------------------------------------------------------------
pqObjectPanel::~pqObjectPanel()
{
}

//-----------------------------------------------------------------------------
pqProxy* pqObjectPanel::referenceProxy() const
{
  return this->ReferenceProxy;
}

//-----------------------------------------------------------------------------
void pqObjectPanel::accept()
{
  pqProxyPanel::accept();

  // this is hacky, but if we are used within a property widget we let
  // our parent handle the proxy modified state. this should be removed
  // when we get rid of the old properties panel
  if(!qobject_cast<pqObjectPanelPropertyWidget *>(this->parent()))
    {
    if(this->ReferenceProxy)
      {
      this->ReferenceProxy->setModifiedState(pqProxy::UNMODIFIED);
      }
    }
}

//-----------------------------------------------------------------------------
void pqObjectPanel::reset()
{
  pqProxyPanel::reset();

  // this is hacky, but if we are used within a property widget we let
  // our parent handle the proxy modified state. this should be removed
  // when we get rid of the old properties panel
  if(!qobject_cast<pqObjectPanelPropertyWidget *>(this->parent()))
    {
    if (this->ReferenceProxy &&
        this->ReferenceProxy->modifiedState() != pqProxy::UNINITIALIZED)
      {
      this->ReferenceProxy->setModifiedState(pqProxy::UNMODIFIED);
      }
    }
}

//-----------------------------------------------------------------------------
void pqObjectPanel::setModified()
{
  // don't change from UNINITIALIZED to MODIFIED
  if(this->ReferenceProxy && this->ReferenceProxy->modifiedState() != pqProxy::UNINITIALIZED)
    {
    this->ReferenceProxy->setModifiedState(pqProxy::MODIFIED);
    pqProxyPanel::setModified();
    }
}

