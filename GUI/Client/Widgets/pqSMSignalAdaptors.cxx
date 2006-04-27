/*=========================================================================

   Program:   ParaQ
   Module:    pqSMSignalAdaptors.cxx

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

// self includes
#include "pqSMSignalAdaptors.h"

// Qt includes

// SM includes
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"

// ParaQ includes
#include "pqSMProxy.h"
#include "pqPipelineData.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"

pqSignalAdaptorProxy::pqSignalAdaptorProxy(QObject* p, 
              const char* colorProperty, const char* signal)
  : QObject(p), PropertyName(colorProperty)
{
  QObject::connect(p, signal,
                   this, SLOT(handleProxyChanged()));
}

QVariant pqSignalAdaptorProxy::proxy() const
{
  QString str = this->parent()->property(this->PropertyName).toString();
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  pqSMProxy p = pm->GetProxy(str.toAscii().data());
  QVariant ret;
  ret.setValue(p);
  return ret;
}

void pqSignalAdaptorProxy::setProxy(const QVariant& var)
{
  if(var != this->proxy())
    {
    pqSMProxy p = var.value<pqSMProxy>();
    /*
    // TODO:  would like to use vtkSMProxyManager
    vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
    QString name = pm->GetProxyName(p);
    */
    pqPipelineSource* o = pqPipelineData::instance()->getModel()->getSourceFor(p);
    if(o)
      {
      QString name = o->GetProxyName();
      this->parent()->setProperty(this->PropertyName, QVariant(name));
      }
    }
}

void pqSignalAdaptorProxy::handleProxyChanged()
{
  QVariant p = this->proxy();
  emit this->proxyChanged(p);
}

