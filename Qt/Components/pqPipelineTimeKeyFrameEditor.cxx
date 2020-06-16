/*=========================================================================

   Program:   ParaView
   Module:    pqPipelineTimeKeyFrameEditor.cxx

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

#include "pqPipelineTimeKeyFrameEditor.h"
#include "ui_pqPipelineTimeKeyFrameEditor.h"

#include <QDoubleValidator>

#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqKeyFrameEditor.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqPipelineTimeKeyFrameEditor::pqInternal
{
public:
  Ui::pqPipelineTimeKeyFrameEditor Ui;
  pqKeyFrameEditor* Editor;
  pqAnimationCue* Cue;
  pqAnimationScene* Scene;
};

//-----------------------------------------------------------------------------
pqPipelineTimeKeyFrameEditor::pqPipelineTimeKeyFrameEditor(
  pqAnimationScene* scene, pqAnimationCue* cue, QWidget* p)
  : QDialog(p)
{
  this->Internal = new pqInternal;
  this->Internal->Ui.setupUi(this);
  QDoubleValidator* val = new QDoubleValidator(this);
  this->Internal->Ui.constantTime->setValidator(val);
  this->Internal->Cue = cue;
  this->Internal->Scene = scene;

  this->Internal->Editor =
    new pqKeyFrameEditor(scene, cue, QString(), this->Internal->Ui.container);
  QHBoxLayout* l = new QHBoxLayout(this->Internal->Ui.container);
  l->setMargin(0);
  l->addWidget(this->Internal->Editor);

  connect(this, SIGNAL(accepted()), this, SLOT(writeKeyFrameData()));

  connect(this->Internal->Ui.noneRadio, SIGNAL(toggled(bool)), this, SLOT(updateState()));
  connect(this->Internal->Ui.constantRadio, SIGNAL(toggled(bool)), this, SLOT(updateState()));
  connect(this->Internal->Ui.variableRadio, SIGNAL(toggled(bool)), this, SLOT(updateState()));

  this->readKeyFrameData();
}

//-----------------------------------------------------------------------------
pqPipelineTimeKeyFrameEditor::~pqPipelineTimeKeyFrameEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPipelineTimeKeyFrameEditor::readKeyFrameData()
{
  this->Internal->Editor->readKeyFrameData();
  int num = this->Internal->Cue->getNumberOfKeyFrames();

  // initialize to current time
  pqServer* server = this->Internal->Scene->getServer();
  pqTimeKeeper* tk = server->getTimeKeeper();
  this->Internal->Ui.constantTime->setText(QString("%1").arg(tk->getTime()));

  if (num < 2)
  {
    this->Internal->Ui.noneRadio->setChecked(true);
    return;
  }

  if (num == 2)
  {
    // could possibly be constant time
    vtkSMProxy* keyFrame = this->Internal->Cue->getKeyFrame(0);
    QVariant val1 = pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyValues"));

    keyFrame = this->Internal->Cue->getKeyFrame(1);
    QVariant val2 = pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyValues"));
    if (val1 == val2)
    {
      this->Internal->Ui.constantRadio->setChecked(true);
      this->Internal->Ui.constantTime->setText(val1.toString());
      return;
    }
  }
  this->Internal->Ui.variableRadio->setChecked(true);
}

//-----------------------------------------------------------------------------
void pqPipelineTimeKeyFrameEditor::writeKeyFrameData()
{
  BEGIN_UNDO_SET("Edit Keyframes");

  vtkSMProxy* cueProxy = this->Internal->Cue->getProxy();
  if (this->Internal->Ui.variableRadio->isChecked())
  {
    this->Internal->Editor->writeKeyFrameData();
    vtkSMPropertyHelper(cueProxy, "UseAnimationTime").Set(0);
  }
  else if (this->Internal->Ui.constantRadio->isChecked())
  {
    vtkSMPropertyHelper(cueProxy, "UseAnimationTime").Set(0);

    int oldNumber = this->Internal->Cue->getNumberOfKeyFrames();
    int newNumber = 2;
    for (int i = 0; i < oldNumber - newNumber; i++)
    {
      this->Internal->Cue->deleteKeyFrame(0);
    }
    for (int i = 0; i < newNumber - oldNumber; i++)
    {
      this->Internal->Cue->insertKeyFrame(0);
    }

    vtkSMProxy* keyFrame = this->Internal->Cue->getKeyFrame(0);
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"), 0);
    pqSMAdaptor::setElementProperty(
      keyFrame->GetProperty("KeyValues"), this->Internal->Ui.constantTime->text());
    keyFrame->UpdateVTKObjects();

    keyFrame = this->Internal->Cue->getKeyFrame(1);
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"), 1);
    pqSMAdaptor::setElementProperty(
      keyFrame->GetProperty("KeyValues"), this->Internal->Ui.constantTime->text());
    keyFrame->UpdateVTKObjects();

    // for convenience, set the current time
    pqServer* server = this->Internal->Scene->getServer();
    pqTimeKeeper* tk = server->getTimeKeeper();
    tk->setTime(this->Internal->Ui.constantTime->text().toDouble());
  }
  else // use animation time
  {
    // remove all keyframes
    int oldNumber = this->Internal->Cue->getNumberOfKeyFrames();
    for (int i = 0; i < oldNumber; i++)
    {
      this->Internal->Cue->deleteKeyFrame(0);
    }
    vtkSMPropertyHelper(cueProxy, "UseAnimationTime").Set(1);
  }
  cueProxy->UpdateVTKObjects();

  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqPipelineTimeKeyFrameEditor::updateState()
{
  this->Internal->Ui.constantTime->setEnabled(false);
  this->Internal->Ui.container->setEnabled(false);

  if (this->Internal->Ui.variableRadio->isChecked())
  {
    this->Internal->Ui.container->setEnabled(true);
  }
  if (this->Internal->Ui.constantRadio->isChecked())
  {
    this->Internal->Ui.constantTime->setEnabled(true);
  }
}
