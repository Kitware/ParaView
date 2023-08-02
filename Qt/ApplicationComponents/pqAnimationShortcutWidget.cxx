// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqAnimationShortcutWidget.h"

#include "pqActiveObjects.h"
#include "pqAnimationCue.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqKeyFrameEditor.h"
#include "pqLineEdit.h"
#include "pqPVApplicationCore.h"
#include "pqSetName.h"
#include "pqUndoStack.h"

#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIntValidator>
#include <QLabel>
#include <QMenu>

#include <cassert>

//-----------------------------------------------------------------------------
pqAnimationShortcutWidget::pqAnimationShortcutWidget(
  QWidget* p, vtkSMProxy* proxy, vtkSMProperty* property)
  : QToolButton(p)
  , Proxy(proxy)
  , Property(property)
  , Scene(nullptr)
{
  QMenu* popupMenu = new QMenu(this);
  popupMenu << pqSetName("AnimationShortcutMenu");
  this->setMenu(popupMenu);
  QObject::connect(popupMenu, SIGNAL(aboutToShow()), SLOT(updateMenu()));
  QObject::connect(popupMenu, SIGNAL(triggered(QAction*)), SLOT(onTriggered(QAction*)));

  this->setPopupMode(QToolButton::InstantPopup);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this, SLOT(updateMenu()));

  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  QObject::connect(
    mgr, SIGNAL(activeSceneChanged(pqAnimationScene*)), this, SLOT(setScene(pqAnimationScene*)));
  this->setScene(mgr->getActiveScene());
}

//-----------------------------------------------------------------------------
pqAnimationShortcutWidget::~pqAnimationShortcutWidget() = default;

//-----------------------------------------------------------------------------
void pqAnimationShortcutWidget::setScene(pqAnimationScene* scene)
{
  if (this->Scene)
  {
    QObject::disconnect(this->Scene, nullptr, this, nullptr);
  }
  this->Scene = scene;
  if (scene)
  {
    QObject::connect(scene, SIGNAL(cuesChanged()), this, SLOT(updateMenu()));
  }
  this->updateMenu();
}

//-----------------------------------------------------------------------------
void pqAnimationShortcutWidget::updateMenu()
{
  QMenu* popupMenu = this->menu();
  assert(popupMenu);

  popupMenu->clear();

  pqAnimationCue* cue = this->Scene ? pqPVApplicationCore::instance()->animationManager()->getCue(
                                        this->Scene, this->Proxy, this->Property->GetXMLName(), 0)
                                    : nullptr;
  if (!cue)
  {
    QAction* playAction = new QAction(
      QIcon(":/pqWidgets/Icons/pqVcrPlay.svg"), tr("Create a new animation track"), this);
    playAction->setData(QVariant(0));
    popupMenu->addAction(playAction);
    this->setIcon(QIcon(":/pqWidgets/Icons/pqVcrPlay.svg"));
  }
  else
  {
    QAction* editAction =
      new QAction(QIcon(":/pqWidgets/Icons/pqRamp24.png"), tr("Edit the animation track"), this);
    editAction->setData(QVariant(1));
    popupMenu->addAction(editAction);
    QAction* deleteAction =
      new QAction(QIcon(":/QtWidgets/Icons/pqDelete.svg"), tr("Remove the animation track"), this);
    deleteAction->setData(QVariant(2));
    popupMenu->addAction(deleteAction);
    this->setIcon(QIcon(":/pqWidgets/Icons/pqRamp24.png"));
  }
}

//-----------------------------------------------------------------------------
void pqAnimationShortcutWidget::onTriggered(QAction* action)
{
  if (!this->Scene)
  {
    return;
  }
  pqAnimationCue* cue = pqPVApplicationCore::instance()->animationManager()->getCue(
    this->Scene, this->Proxy, this->Property->GetXMLName(), 0);
  if (action->data().toInt() == 2)
  {
    BEGIN_UNDO_SET(tr("Remove Animation Track"));
    this->Scene->removeCue(cue);
    END_UNDO_SET();
    return;
  }

  // Recover the scene and create the cue
  bool existingCue = true;
  if (!cue)
  {
    existingCue = false;
    BEGIN_UNDO_SET(tr("Add Animation Track"));
    cue = this->Scene->createCue(this->Proxy, this->Property->GetXMLName(), 0);
    END_UNDO_SET();
  }

  // Show a dialog with a pqKeyFrameEditor and a NumberOfFrames LineEdit
  QDialog* dialog = new QDialog(pqCoreUtilities::mainWidget());
  QGridLayout* layout = new QGridLayout(dialog);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  pqKeyFrameEditor* editor = new pqKeyFrameEditor(this->Scene, cue, QString(), dialog);

  layout->addWidget(editor, 0, 0, 1, 2);

  connect(dialog, SIGNAL(accepted()), editor, SLOT(writeKeyFrameData()));
  dialog->setWindowTitle(tr("Animation Keyframes"));
  dialog->resize(600, 400);

  QString mode(vtkSMPropertyHelper(this->Scene->getProxy(), "PlayMode").GetAsString());
  pqLineEdit* animationLineEdit = new pqLineEdit(dialog);
  animationLineEdit->setValidator(
    new QIntValidator(1, static_cast<int>(~0u >> 1), animationLineEdit));
  QLabel* label = nullptr;
  if (mode == "Sequence")
  {
    label = new QLabel(tr("No. frames:"), dialog);
    animationLineEdit->setText(
      QString::number(vtkSMPropertyHelper(this->Scene->getProxy(), "NumberOfFrames").GetAsInt()));
  }
  else if (mode == "Real Time")
  {
    label = new QLabel(tr("Duration (s):"), dialog);
    animationLineEdit->setText(
      QString::number(vtkSMPropertyHelper(this->Scene->getProxy(), "Duration").GetAsInt()));
  }
  if (label)
  {
    layout->addWidget(label, 1, 0);
    layout->addWidget(animationLineEdit, 1, 1);
  }

  connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), dialog, SLOT(reject()));
  layout->addWidget(buttons, 2, 0, 1, 2);

  dialog->setAttribute(Qt::WA_QuitOnClose, false);
  if (dialog->exec() != QDialog::Accepted && !existingCue)
  {
    // Remove the cue if the dialog have been cancelled
    this->Scene->removeCue(cue);
    return;
  }

  if (mode == "Sequence")
  {
    vtkSMPropertyHelper(this->Scene->getProxy(), "NumberOfFrames")
      .Set(animationLineEdit->text().toInt());
    this->Scene->getProxy()->UpdateProperty("NumberOfFrames");
  }
  else if (mode == "Real Time")
  {
    vtkSMPropertyHelper(this->Scene->getProxy(), "Duration").Set(animationLineEdit->text().toInt());
    this->Scene->getProxy()->UpdateProperty("Duration");
  }
  delete dialog;
}
