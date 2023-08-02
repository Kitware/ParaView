// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
  l->setContentsMargins(0, 0, 0, 0);
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
  BEGIN_UNDO_SET(tr("Edit Keyframes"));

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
