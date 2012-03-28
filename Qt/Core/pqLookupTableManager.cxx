/*=========================================================================

   Program: ParaView
   Module:    pqLookupTableManager.cxx

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
#include "pqLookupTableManager.h"

#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqRenderViewBase.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqScalarOpacityFunction.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
pqLookupTableManager::pqLookupTableManager(QObject* _parent/*=0*/)
  : QObject(_parent)
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onAddProxy(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onRemoveProxy(pqProxy*)));
}

//-----------------------------------------------------------------------------
pqLookupTableManager::~pqLookupTableManager()
{
}

//-----------------------------------------------------------------------------
void pqLookupTableManager::onAddProxy(pqProxy* proxy)
{
  if (pqScalarsToColors* lut = qobject_cast<pqScalarsToColors*>(proxy))
    {
    this->onAddLookupTable(lut);
    }
  else if (pqScalarOpacityFunction* opf = qobject_cast<pqScalarOpacityFunction*>(proxy))
    {
    this->onAddOpacityFunction(opf);
    }
}

//-----------------------------------------------------------------------------
void pqLookupTableManager::onRemoveProxy(pqProxy* proxy)
{
  if (pqScalarsToColors* lut = qobject_cast<pqScalarsToColors*>(proxy))
    {
    this->onRemoveLookupTable(lut);
    }
  else if (pqScalarOpacityFunction* opf = qobject_cast<pqScalarOpacityFunction*>(proxy))
    {
    this->onRemoveOpacityFunction(opf);
    }
}

//-----------------------------------------------------------------------------
pqScalarBarRepresentation* pqLookupTableManager::setScalarBarVisibility(
  pqDataRepresentation* repr,  bool visible)
{

  pqView *view = repr->getView();  
  pqScalarsToColors* stc = repr->getLookupTable();  
  if (!stc || !view)
    {
    qCritical() << "Arguments  to pqLookupTableManager::setScalarBarVisibility "
      "cannot be null";
    return NULL;
    }

  pqRenderViewBase* renderView = qobject_cast<pqRenderViewBase*>(view);
  if (!renderView)
    {
    qWarning() << "Scalar bar cannot be created for the view specified";
    return NULL;
    }

  pqScalarBarRepresentation* sb = stc->getScalarBar(renderView);
  if (!sb && !visible)
    {
    // nothing to do, scalar bar already invisible.
    return NULL;
    }

  if (!sb)
    {
    // No scalar bar exists currently, so we create a new one.
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    sb = builder->createScalarBarDisplay(stc, renderView);
    this->initialize(sb);
    
    //fill the proper component name into the label
    QString arrayName;    
    int numComponents;
    int component;    
    if ( this->getLookupTableProperties( stc, arrayName, numComponents, component ) )
      {  
      int field = repr->getProxyScalarMode( );
      QString compName = repr->getComponentName( arrayName.toAscii(), field, component );
      sb->setTitle( arrayName, compName );
      }
    }

  if (sb)
    {
    sb->setVisible(visible);
    }
  else
    {
    qDebug() << "Failed to locate/create scalar bar.";
    }
  return sb;
}
