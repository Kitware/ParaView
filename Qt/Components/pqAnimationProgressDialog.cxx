/*=========================================================================

   Program: ParaView
   Module:  pqAnimationProgressDialog.cxx

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
