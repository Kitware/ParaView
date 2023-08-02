// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
