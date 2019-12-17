/*=========================================================================

   Program:   ParaView
   Module:    pqAnimationShortcutWidget.cxx

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
pqAnimationShortcutWidget::~pqAnimationShortcutWidget()
{
}

//-----------------------------------------------------------------------------
void pqAnimationShortcutWidget::setScene(pqAnimationScene* scene)
{
  if (this->Scene)
  {
    QObject::disconnect(this->Scene, 0, this, 0);
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

  pqAnimationCue* cue = this->Scene
    ? pqPVApplicationCore::instance()->animationManager()->getCue(
        this->Scene, this->Proxy, this->Property->GetXMLName(), 0)
    : nullptr;
  if (!cue)
  {
    QAction* playAction = new QAction(
      QIcon(":/pqWidgets/Icons/pqVcrPlay24.png"), tr("Create a new animation track"), this);
    playAction->setData(QVariant(0));
    popupMenu->addAction(playAction);
    this->setIcon(QIcon(":/pqWidgets/Icons/pqVcrPlay24.png"));
  }
  else
  {
    QAction* editAction =
      new QAction(QIcon(":/pqWidgets/Icons/pqRamp24.png"), tr("Edit the animation track"), this);
    editAction->setData(QVariant(1));
    popupMenu->addAction(editAction);
    QAction* deleteAction = new QAction(
      QIcon(":/QtWidgets/Icons/pqDelete16.png"), tr("Remove the animation track"), this);
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
    BEGIN_UNDO_SET("Remove Animation Track");
    this->Scene->removeCue(cue);
    END_UNDO_SET();
    return;
  }

  // Recover the scene and create the cue
  bool existingCue = true;
  if (!cue)
  {
    existingCue = false;
    BEGIN_UNDO_SET("Add Animation Track");
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
  QLabel* label = NULL;
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
