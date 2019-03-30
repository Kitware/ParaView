/*=========================================================================

  Program:   ParaView
  Module:    pqPythonView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqPythonView.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPythonViewProxy.h"
#include "vtkSMSourceProxy.h"

// Qt Includes.
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QtDebug>

// ParaView Includes.
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqQVTKWidget.h"
#include "pqServer.h"

#include <cassert>
#include <string>

class pqPythonView::pqInternal
{
public:
  QPoint MouseOrigin;
  bool InitializedWidgets;

  pqInternal() { this->InitializedWidgets = false; }
  ~pqInternal() {}
};

//-----------------------------------------------------------------------------
pqPythonView::pqPythonView(const QString& type, const QString& group, const QString& name,
  vtkSMViewProxy* renViewProxy, pqServer* server, QObject* _parent /*=NULL*/)
  : Superclass(type, group, name, renViewProxy, server, _parent)
{
  this->Internal = new pqPythonView::pqInternal();
}

//-----------------------------------------------------------------------------
pqPythonView::~pqPythonView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMPythonViewProxy* pqPythonView::getPythonViewProxy()
{
  return vtkSMPythonViewProxy::SafeDownCast(this->getViewProxy());
}

//-----------------------------------------------------------------------------
void pqPythonView::setPythonScript(QString& source)
{
  vtkSMPropertyHelper(this->getProxy(), "Script").Set(source.toStdString().c_str());
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QString pqPythonView::getPythonScript()
{
  const char* scriptString = vtkSMPropertyHelper(this->getProxy(), "Script", true).GetAsString();
  if (scriptString)
  {
    return QString(scriptString);
  }

  return QString();
}

//-----------------------------------------------------------------------------
QWidget* pqPythonView::createWidget()
{
  vtkSMViewProxy* viewProxy = this->getViewProxy();
  assert(viewProxy);

  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setViewProxy(viewProxy);
  vtkwidget->setContextMenuPolicy(Qt::NoContextMenu);
  vtkwidget->installEventFilter(this);

  vtkwidget->setRenderWindow(viewProxy->GetRenderWindow());
  viewProxy->SetupInteractor(vtkwidget->interactor());
  return vtkwidget;
}

//-----------------------------------------------------------------------------
bool pqPythonView::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
    {
      this->Internal->MouseOrigin = me->pos();
    }
  }
  else if (e->type() == QEvent::MouseMove && !this->Internal->MouseOrigin.isNull())
  {
    QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
    QPoint delta = newPos - this->Internal->MouseOrigin;
    if (delta.manhattanLength() < 3)
    {
      this->Internal->MouseOrigin = QPoint();
    }
  }
  else if (e->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton && !this->Internal->MouseOrigin.isNull())
    {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Internal->MouseOrigin;
      if (delta.manhattanLength() < 3 && qobject_cast<QWidget*>(caller))
      {
        QList<QAction*> actions = this->widget()->actions();
        if (!actions.isEmpty())
        {
          QMenu* menu = new QMenu(this->widget());
          menu->setAttribute(Qt::WA_DeleteOnClose);
          menu->addActions(actions);
          menu->popup(qobject_cast<QWidget*>(caller)->mapToGlobal(newPos));
        }
      }
      this->Internal->MouseOrigin = QPoint();
    }
  }
  return Superclass::eventFilter(caller, e);
}
