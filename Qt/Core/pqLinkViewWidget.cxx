// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqLinkViewWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include "pqApplicationCore.h"
#include "pqLinksModel.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#define pqCheckBoxSignal checkStateChanged
using pqCheckState = Qt::CheckState;
#else
#define pqCheckBoxSignal stateChanged
using pqCheckState = int;
#endif

//-----------------------------------------------------------------------------
pqLinkViewWidget::pqLinkViewWidget(pqRenderView* firstLink)
  : QWidget(firstLink->widget(), Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
  , RenderView(firstLink)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  QLabel* label = new QLabel(this);
  l->addWidget(label);
  label->setText(tr("Click on another view to link with."));
  label->setWordWrap(true);
  QHBoxLayout* hl = new QHBoxLayout;
  l->addLayout(hl);
  label = new QLabel(tr("Name:"), this);
  hl->addWidget(label);
  this->LineEdit = new QLineEdit(this);
  hl->addWidget(this->LineEdit);
  this->InteractiveViewLinkCheckBox = new QCheckBox(tr("Interactive View Link"), this);
  l->addWidget(this->InteractiveViewLinkCheckBox);
  this->CameraWidgetViewLinkCheckBox = new QCheckBox(tr("Camera Widget View Link"), this);
  l->addWidget(this->CameraWidgetViewLinkCheckBox);

  QObject::connect(
    this->InteractiveViewLinkCheckBox, &QCheckBox::pqCheckBoxSignal, this,
    [this](pqCheckState state)
    { this->CameraWidgetViewLinkCheckBox->setEnabled(state == Qt::Unchecked); },
    Qt::QueuedConnection);

  QObject::connect(
    this->CameraWidgetViewLinkCheckBox, &QCheckBox::pqCheckBoxSignal, this,
    [this](pqCheckState state)
    { this->InteractiveViewLinkCheckBox->setEnabled(state == Qt::Unchecked); },
    Qt::QueuedConnection);

  QPushButton* button = new QPushButton(this);
  l->addWidget(button);
  button->setText(tr("Cancel"));
  QObject::connect(button, SIGNAL(clicked(bool)), this, SLOT(close()));

  int index = 0;
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QString name = tr("CameraLink%1").arg(index);
  while (model->getLink(name))
  {
    name = tr("CameraLink%1").arg(++index);
  }
  this->LineEdit->setText(name);
  this->LineEdit->selectAll();
}

//-----------------------------------------------------------------------------
pqLinkViewWidget::~pqLinkViewWidget() = default;

//-----------------------------------------------------------------------------
bool pqLinkViewWidget::event(QEvent* e)
{
  if (e->type() == QEvent::Show)
  {
    // window shown, watch events
    QApplication::instance()->installEventFilter(this);
    this->LineEdit->setFocus(Qt::OtherFocusReason);
  }
  else if (e->type() == QEvent::Hide)
  {
    // window hidden, done watching events
    QApplication::instance()->removeEventFilter(this);
  }
  return QWidget::event(e);
}

//-----------------------------------------------------------------------------
bool pqLinkViewWidget::eventFilter(QObject* watched, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick)
  {
    pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

    if (watched->inherits("QWidgetWindow"))
    {
      // Pass down to the actual widget
      return QObject::eventFilter(watched, e);
    }

    QWidget* wid = qobject_cast<QWidget*>(watched);
    pqRenderView* otherView = nullptr;

    QList<pqRenderView*> views = smModel->findItems<pqRenderView*>();
    Q_FOREACH (pqRenderView* view, views)
    {
      if (view && wid && view->widget() == wid->parent())
      {
        otherView = view;
        break;
      }
    }

    // if the user clicked on another view
    if (otherView && otherView != this->RenderView)
    {
      QString name = this->LineEdit->text();
      pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
      vtkSMLink* link = model->getLink(name);
      if (link)
      {
        model->removeLink(name);
      }

      if (this->CameraWidgetViewLinkCheckBox->isChecked())
      {
        model->addCameraWidgetLink(name, this->RenderView->getProxy(), otherView->getProxy());
      }
      else
      {
        model->addCameraLink(name, this->RenderView->getProxy(), otherView->getProxy(),
          this->InteractiveViewLinkCheckBox->isChecked());
      }

      this->close();
    }
    // if the user didn't click in this widget
    else if (!watched || (watched != this && watched->parent() != this))
    {
      // consume invalid mouse events
      return true;
    }
  }
  return QObject::eventFilter(watched, e);
}
