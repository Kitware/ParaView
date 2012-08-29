/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqQuadView.h"

#include "pqProxy.h"
#include "pqQVTKWidget.h"
#include "vtkPVQuadRenderView.h"
#include "vtkSMProxy.h"

#include <QGridLayout>
#include <QWidget>

//-----------------------------------------------------------------------------
pqQuadView::pqQuadView(
   const QString& viewType, const QString& group, const QString& name,
    vtkSMViewProxy* viewProxy, pqServer* server, QObject* p)
  : Superclass(viewType, group, name, viewProxy, server, p)
{
}

//-----------------------------------------------------------------------------
pqQuadView::~pqQuadView()
{
}

//-----------------------------------------------------------------------------
QWidget* pqQuadView::createWidget()
{
  vtkPVQuadRenderView* clientView = vtkPVQuadRenderView::SafeDownCast(
    this->getProxy()->GetClientSideObject());

  QWidget* container = new QWidget();
  container->setObjectName("QuadView");
  container->setStyleSheet("background-color: white");
  container->setAutoFillBackground(true);

  QGridLayout* gLayout = new QGridLayout(container);
  gLayout->setSpacing(2);
  gLayout->setContentsMargins(0,0,0,0);

  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setSizePropertyName("ViewSizeTopLeft");
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->SetRenderWindow(clientView->GetOrthoViewWindow(vtkPVQuadRenderView::TOP_LEFT));
  gLayout->addWidget(vtkwidget, 0, 0);

  vtkwidget = new pqQVTKWidget();
  vtkwidget->setSizePropertyName("ViewSizeBottomLeft");
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->SetRenderWindow(clientView->GetOrthoViewWindow(vtkPVQuadRenderView::BOTTOM_LEFT));
  gLayout->addWidget(vtkwidget, 1, 0);

  vtkwidget = new pqQVTKWidget();
  vtkwidget->setSizePropertyName("ViewSizeTopRight");
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->SetRenderWindow(clientView->GetOrthoViewWindow(vtkPVQuadRenderView::TOP_RIGHT));
  gLayout->addWidget(vtkwidget, 0, 1);

  vtkwidget = qobject_cast<pqQVTKWidget*>(this->Superclass::createWidget());
  vtkwidget->setParent(container);
  vtkwidget->setObjectName("View3D");
  vtkwidget->SetRenderWindow(clientView->GetRenderWindow());
  gLayout->addWidget(vtkwidget, 1, 1);

  return container;
}
