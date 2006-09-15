/*=========================================================================

   Program: ParaView
   Module:    pqDisplay.cxx

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

/// \file pqDisplay.cxx
/// \date 4/24/2006

#include "pqDisplay.h"


// ParaView Server Manager includes.
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h" 

// Qt includes.
#include <QPointer>
#include <QList>
#include <QtDebug>

// ParaView includes.
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"


//-----------------------------------------------------------------------------
class pqDisplayInternal
{
public:
  // Set of render modules showing this display. Typically,
  // it will be 1, but theoretically there can be more.
  QList<QPointer<pqRenderModule> > RenderModules;
};

//-----------------------------------------------------------------------------
pqDisplay::pqDisplay(const QString& group, const QString& name,
  vtkSMProxy* display,
  pqServer* server, QObject* p/*=null*/):
  pqProxy(group, name, display, server, p)
{
  this->Internal = new pqDisplayInternal();
}

//-----------------------------------------------------------------------------
pqDisplay::~pqDisplay()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqDisplay::shownIn(pqRenderModule* rm) const
{
  return this->Internal->RenderModules.contains(rm);
}

//-----------------------------------------------------------------------------
void pqDisplay::addRenderModule(pqRenderModule* rm)
{
  if (!this->Internal->RenderModules.contains(rm))
    {
    this->Internal->RenderModules.push_back(rm);
    }
}

//-----------------------------------------------------------------------------
void pqDisplay::removeRenderModule(pqRenderModule* rm)
{
  if (this->Internal->RenderModules.contains(rm))
    {
    this->Internal->RenderModules.removeAll(rm);
    }
}

//-----------------------------------------------------------------------------
unsigned int pqDisplay::getNumberOfRenderModules() const
{
  return this->Internal->RenderModules.size();
}

//-----------------------------------------------------------------------------
pqRenderModule* pqDisplay::getRenderModule(unsigned int index) const
{
  if (index >= this->getNumberOfRenderModules())
    {
    qDebug() << "Invalid index : " << index;
    return NULL;
    }
  return this->Internal->RenderModules[index];
}

//-----------------------------------------------------------------------------
void pqDisplay::renderAllViews(bool force /*=false*/)
{
  foreach(pqRenderModule* rm, this->Internal->RenderModules)
    {
    if (rm)
      {
      if (force)
        {
        rm->forceRender();
        }
      else
        {
        rm->render();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqDisplay::onVisibilityChanged()
{
  emit this->visibilityChanged(this->isVisible());
}

