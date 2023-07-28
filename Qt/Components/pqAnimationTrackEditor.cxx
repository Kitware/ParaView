// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqAnimationTrackEditor.h"

#include "pqAnimatableProxyComboBox.h"
#include "pqAnimationCue.h"
#include "pqAnimationManager.h"
#include "pqCoreUtilities.h"
#include "pqKeyFrameEditor.h"
#include "pqPipelineTimeKeyFrameEditor.h"
#if VTK_MODULE_ENABLE_ParaView_pqPython
#include <pqPythonTextArea.h>
#endif
#include "vtkPVCameraCueManipulator.h"
#include "vtkSMPropertyHelper.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QErrorMessage>
#include <QFormLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

struct pqAnimationTrackEditor::pqInternals
{
  pqAnimationScene* Scene = nullptr;
  QDialog* Editor = nullptr;
  vtkSMProxy* SelectedSource = nullptr;

  pqInternals(pqAnimationScene* scene)
    : Scene(scene)
  {
    this->Editor = new QDialog(pqCoreUtilities::mainWidget());
    QDialogButtonBox* buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), this->Editor, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this->Editor, SLOT(reject()));

    QVBoxLayout* dialogLayout = new QVBoxLayout(this->Editor);
    dialogLayout->addWidget(buttons);
  }

  QVBoxLayout* vLayout() { return dynamic_cast<QVBoxLayout*>(this->Editor->layout()); }

  void showTimeKeeperEditor(pqAnimationCue* cue)
  {
    if (this->Editor)
    {
      this->Editor->deleteLater();
    }

    this->Editor = new pqPipelineTimeKeyFrameEditor(this->Scene, cue, nullptr);
    this->Editor->setAttribute(Qt::WA_QuitOnClose, false);
    this->Editor->setAttribute(Qt::WA_DeleteOnClose);
    this->Editor->show();
  }

  void showPythonEditor(pqAnimationCue* cue)
  {
#if VTK_MODULE_ENABLE_ParaView_pqPython
    this->Editor->setWindowTitle(tr("Edit Python cue"));
    auto pythonArea = new pqPythonTextArea(this->Editor);
    pythonArea->setText(vtkSMPropertyHelper(cue->getProxy(), "Script").GetAsString());
    this->vLayout()->insertWidget(0, pythonArea);

    if (this->Editor->exec() == QDialog::Accepted)
    {
      vtkSMPropertyHelper(cue->getProxy(), "Script")
        .Set(pythonArea->getTextEdit()->toPlainText().toUtf8().data());
      cue->getProxy()->UpdateVTKObjects();
    }
#else
    Q_UNUSED(cue);
    auto error = new QErrorMessage();
    error->showMessage(tr("Cannot edit track: python is not supported"));
#endif
  }

  void showCameraFollowDataEditor(pqAnimationCue* cue)
  {
    // show a combo-box allowing the user to select the data source to follow
    QFormLayout* formLayout = new QFormLayout;
    pqAnimatableProxyComboBox* comboBox = new pqAnimatableProxyComboBox(this->Editor);
    this->SelectedSource = vtkSMPropertyHelper(cue->getProxy(), "DataSource").GetAsProxy();
    comboBox->setCurrentIndex(comboBox->findProxy(this->SelectedSource));
    connect(comboBox, &pqAnimatableProxyComboBox::currentProxyChanged,
      [&](vtkSMProxy* proxy) { this->SelectedSource = proxy; });

    formLayout->addRow(tr("Data Source to Follow:"), comboBox);
    this->vLayout()->insertLayout(0, formLayout);
    this->Editor->setWindowTitle(tr("Select Data Source"));

    if (this->Editor->exec() == QDialog::Accepted)
    {
      vtkSMPropertyHelper helper(cue->getProxy(), "DataSource");
      if (helper.GetAsProxy() != this->SelectedSource)
      {
        helper.Set(this->SelectedSource);
        cue->getProxy()->UpdateVTKObjects();
      }
    }
  }

  void showKeyFrameEditor(pqAnimationCue* cue)
  {
    pqKeyFrameEditor* editor =
      new pqKeyFrameEditor(this->Scene, cue, tr("Editing ") + cue->getDisplayName(), this->Editor);

    this->vLayout()->insertWidget(0, editor);

    auto buttons = this->Editor->findChild<QDialogButtonBox*>();
    auto apply = buttons->addButton(QDialogButtonBox::Apply);
    QObject::connect(apply, &QPushButton::clicked, editor, &pqKeyFrameEditor::writeKeyFrameData);
    QObject::connect(apply, &QPushButton::clicked, [=]() { apply->setEnabled(false); });
    QObject::connect(editor, &pqKeyFrameEditor::modified, [=]() { apply->setEnabled(true); });

    connect(this->Editor, SIGNAL(accepted()), editor, SLOT(writeKeyFrameData()));

    this->Editor->setWindowTitle(tr("Animation Keyframes"));
    this->Editor->show();
  }
};

//-----------------------------------------------------------------------------
pqAnimationTrackEditor::pqAnimationTrackEditor(pqAnimationScene* scene, pqAnimationCue* parentCue)
  : Superclass(parentCue)
  , Internals(new pqAnimationTrackEditor::pqInternals(scene))
{
}

//-----------------------------------------------------------------------------
pqAnimationTrackEditor::~pqAnimationTrackEditor() = default;

//-----------------------------------------------------------------------------
void pqAnimationTrackEditor::showEditor()
{
  auto cue = dynamic_cast<pqAnimationCue*>(this->parent());

  if (!cue)
  {
    return;
  }

  if (cue->isTimekeeperCue())
  {
    this->Internals->showTimeKeeperEditor(cue);
  }
  else if (cue->isPythonCue())
  {
    this->Internals->showPythonEditor(cue);
  }
  else if (cue->isCameraCue() &&
    vtkSMPropertyHelper(cue->getProxy(), "Mode").GetAsInt() ==
      vtkPVCameraCueManipulator::FOLLOW_DATA)
  {
    this->Internals->showCameraFollowDataEditor(cue);
  }
  else
  {
    this->Internals->showKeyFrameEditor(cue);
  }
}
