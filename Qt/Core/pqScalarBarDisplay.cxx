/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarDisplay.cxx

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
#include "pqScalarBarDisplay.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QtDebug>
#include <QPointer>

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqScalarBarDisplayInternal
{
public:
  QPointer<pqScalarsToColors> LookupTable;
  vtkEventQtSlotConnect* VTKConnect;
};

//-----------------------------------------------------------------------------
pqScalarBarDisplay::pqScalarBarDisplay(const QString& group, const QString& name,
    vtkSMProxy* scalarbar, pqServer* server,
    QObject* _parent)
: pqDisplay(group, name, scalarbar, server, _parent)
{
  this->Internal = new pqScalarBarDisplayInternal;

  this->Internal->VTKConnect = vtkEventQtSlotConnect::New();
  this->Internal->VTKConnect->Connect(scalarbar->GetProperty("LookupTable"),
    vtkCommand::ModifiedEvent, this, SLOT(onLookupTableModified()));

  // load default values.
  this->onLookupTableModified();
}

//-----------------------------------------------------------------------------
pqScalarBarDisplay::~pqScalarBarDisplay()
{
  this->Internal->VTKConnect->Disconnect();
  this->Internal->VTKConnect->Delete();

  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqScalarBarDisplay::getLookupTable() const
{
  return this->Internal->LookupTable;
}

//-----------------------------------------------------------------------------
void pqScalarBarDisplay::onLookupTableModified()
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxy* curLUTProxy = 
    pqSMAdaptor::getProxyProperty(this->getProxy()->GetProperty("LookupTable"));
  pqScalarsToColors* curLUT = qobject_cast<pqScalarsToColors*>(
    smmodel->getPQProxy(curLUTProxy));

  if (curLUT == this->Internal->LookupTable)
    {
    return;
    }

  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->removeScalarBar(this);
    }

  this->Internal->LookupTable = curLUT;
  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->addScalarBar(this);
    }
}

//-----------------------------------------------------------------------------
bool pqScalarBarDisplay::isVisible() const
{
  int visible = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("Visibility")).toInt();
  return (visible != 0);
}

//-----------------------------------------------------------------------------
void pqScalarBarDisplay::setVisible(bool visible)
{
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Visibility"),
    (visible? 1 : 0));
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
