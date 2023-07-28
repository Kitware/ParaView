// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimationProgressDialog.h"

#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqAnimationProgressDialog::pqAnimationProgressDialog(const QString& labelTextArg,
  const QString& cancelButtonTextArg, int minimumArg, int maximumArg, QWidget* parentArg,
  Qt::WindowFlags f)
  : Superclass(labelTextArg, cancelButtonTextArg, minimumArg, maximumArg, parentArg, f)
{
  this->setWindowModality(Qt::ApplicationModal);
}

//-----------------------------------------------------------------------------
pqAnimationProgressDialog::pqAnimationProgressDialog(QWidget* parentArg, Qt::WindowFlags f)
  : Superclass(parentArg, f)

{
  this->setWindowModality(Qt::ApplicationModal);
}

//-----------------------------------------------------------------------------
pqAnimationProgressDialog::~pqAnimationProgressDialog()
{
  QObject::disconnect(this->Connection);
}

//-----------------------------------------------------------------------------
void pqAnimationProgressDialog::setAnimationScene(pqAnimationScene* scene)
{
  if (!scene)
  {
    return;
  }

  QPointer<pqAnimationScene> qscene(scene);
  QObject::connect(this, &QProgressDialog::canceled, [qscene, this]() {
    this->hide();
    if (qscene)
    {
      qscene->getProxy()->InvokeCommand("Stop");
    }
  });

  this->Connection =
    QObject::connect(scene, &pqAnimationScene::tick, [this](int progressInPercent) {
      if (this->isVisible())
      {
        const auto delta = this->maximum() - this->minimum();
        this->setValue(this->minimum() + (progressInPercent * delta) / 100);
      }
    });
}

//-----------------------------------------------------------------------------
void pqAnimationProgressDialog::setAnimationScene(vtkSMProxy* scene)
{
  auto appcore = pqApplicationCore::instance();
  auto pqscene = appcore->getServerManagerModel()->findItem<pqAnimationScene*>(scene);
  this->setAnimationScene(pqscene);
}
