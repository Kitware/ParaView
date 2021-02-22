/*=========================================================================

   Program: ParaView
   Module:    pqRepresentation.cxx

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
#include "pqRepresentation.h"

// ParaView Server Manager includes.
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

// Qt includes.
#include <QPointer>

// ParaView includes.
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqView.h"

//-----------------------------------------------------------------------------
class pqRepresentation::pqInternal
{
public:
  QPointer<pqView> View;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqRepresentation::pqRepresentation(const QString& group, const QString& name, vtkSMProxy* repr,
  pqServer* server, QObject* _parent /*=nullptr*/)
  : pqProxy(group, name, repr, server, _parent)
{
  this->Internal = new pqRepresentation::pqInternal();
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  // vtkCommand::EndEvent is fired when the representation is updated.
  this->Internal->VTKConnect->Connect(repr, vtkCommand::EndEvent, this, SIGNAL(updated()));
  if (repr->GetProperty("Visibility"))
  {
    this->Internal->VTKConnect->Connect(repr->GetProperty("Visibility"), vtkCommand::ModifiedEvent,
      this, SLOT(onVisibilityChanged()), nullptr, 0, Qt::QueuedConnection);
  }
}

//-----------------------------------------------------------------------------
pqRepresentation::~pqRepresentation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqRepresentation::setView(pqView* view)
{
  this->Internal->View = view;
  Q_EMIT this->visibilityChanged(false);
}

//-----------------------------------------------------------------------------
pqView* pqRepresentation::getView() const
{
  return this->Internal->View;
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqRepresentation::getViewProxy() const
{
  return (this->Internal->View ? this->Internal->View->getViewProxy() : nullptr);
}

//-----------------------------------------------------------------------------
void pqRepresentation::renderView(bool force)
{
  if (this->Internal->View)
  {
    if (force)
    {
      this->Internal->View->forceRender();
    }
    else
    {
      this->Internal->View->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqRepresentation::onVisibilityChanged()
{
  Q_EMIT this->visibilityChanged(this->isVisible());
}

//-----------------------------------------------------------------------------
bool pqRepresentation::isVisible() const
{
  int visible =
    pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("Visibility")).toInt();
  return (visible != 0);
}

//-----------------------------------------------------------------------------
void pqRepresentation::setVisible(bool visible)
{
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Visibility"), (visible ? 1 : 0));
  this->getProxy()->UpdateVTKObjects();
}
