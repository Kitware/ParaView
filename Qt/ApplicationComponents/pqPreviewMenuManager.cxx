/*=========================================================================

   Program: ParaView
   Module:  pqPreviewMenuManager.cxx

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
#include "pqPreviewMenuManager.h"
#include "ui_pqCustomResolutionDialog.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqUndoStack.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewLayoutProxy.h"

#include <QDialog>
#include <QIntValidator>
#include <QMenu>
#include <QRegExp>
#include <QStringList>

#include <cassert>

#define SETUP_ACTION(actn)                                                                         \
  if (QAction* tmp = actn)                                                                         \
  {                                                                                                \
    tmp->setCheckable(true);                                                                       \
    this->connect(tmp, SIGNAL(triggered(bool)), SLOT(lockResolution(bool)));                       \
  }

namespace
{
QString generateText(int dx, int dy, const QString& label = QString())
{
  return label.isEmpty() ? QString("%1 x %2").arg(dx).arg(dy)
                         : QString("%1 x %2 (%3)").arg(dx).arg(dy).arg(label);
}

QString extractLabel(const QString& txt)
{
  QRegExp re("^(\\d+) x (\\d+) \\((.*)\\)$");
  if (re.indexIn(txt) != -1)
  {
    return re.cap(3);
  }
  return QString();
}

QSize extractSize(const QString& txt)
{
  QRegExp re("^(\\d+) x (\\d+)");
  if (re.indexIn(txt) != -1)
  {
    return QSize(re.cap(1).toInt(), re.cap(2).toInt());
  }
  return QSize();
}
}

//-----------------------------------------------------------------------------
pqPreviewMenuManager::pqPreviewMenuManager(QMenu* menu)
  : Superclass(menu)
  , FirstCustomAction(nullptr)
{
  QStringList defaultItems;
  defaultItems << "1280 x 720 (HD)"
               << "1280 x 800 (WXGA)"
               << "1280 x 1024 (SXGA)"
               << "1600 x 900 (HD+)"
               << "1920 x 1080 (FHD)"
               << "3840 x 2160 (4K UHD)";
  this->init(defaultItems, menu);
  this->connect(menu, SIGNAL(aboutToShow()), SLOT(aboutToShow()));
}

//-----------------------------------------------------------------------------
pqPreviewMenuManager::pqPreviewMenuManager(const QStringList& defaultItems, QMenu* menu)
  : Superclass(menu)
  , FirstCustomAction(nullptr)
{
  this->init(defaultItems, menu);
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::init(const QStringList& defaultItems, QMenu* menu)
{
  foreach (const QString& txt, defaultItems)
  {
    SETUP_ACTION(menu->addAction(txt));
  }
  if (defaultItems.size() > 0)
  {
    menu->addSeparator();
  }
  menu->addAction("Custom ...", this, SLOT(addCustom()));

  this->Timer.setSingleShot(true);
  this->Timer.setInterval(500);
  this->Timer.connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), SLOT(start()));
  this->connect(&this->Timer, SIGNAL(timeout()), SLOT(updateEnabledState()));
  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqPreviewMenuManager::~pqPreviewMenuManager()
{
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::updateEnabledState()
{
  QMenu* menu = this->parentMenu();
  menu->setEnabled(pqActiveObjects::instance().activeLayout() != nullptr);
}

//-----------------------------------------------------------------------------
QMenu* pqPreviewMenuManager::parentMenu() const
{
  return qobject_cast<QMenu*>(this->parent());
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::updateCustomActions()
{
  QMenu* menu = this->parentMenu();

  // remove old custom actions.
  QList<QAction*> actions = menu->actions();
  for (int index = 0; index < actions.size(); ++index)
  {
    if (actions[index]->data().toBool())
    {
      menu->removeAction(actions[index]);
    }
  }

  this->FirstCustomAction = nullptr;

  // add custom actions.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList resolutions = settings->value("PreviewResolutions").toStringList();
  foreach (const QString& res, resolutions)
  {
    QAction* actn = menu->addAction(res);
    SETUP_ACTION(actn);
    actn->setData(true); // flag custom actions.

    // save for later.
    if (this->FirstCustomAction == nullptr)
    {
      this->FirstCustomAction = actn;
    }
  }
}

//-----------------------------------------------------------------------------
bool pqPreviewMenuManager::prependCustomResolution(int dx, int dy, const QString& label)
{
  if (dx >= 1 && dy >= 1)
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList resolutions = settings->value("PreviewResolutions").toStringList();

    QString txt = generateText(dx, dy, label);
    // find and remove duplicate, if any.
    resolutions.removeOne(txt);
    resolutions.push_front(txt);
    while (resolutions.size() > 5)
    {
      resolutions.pop_back();
    }
    settings->setValue("PreviewResolutions", resolutions);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::addCustom()
{
  QDialog dialog(pqCoreUtilities::mainWidget());
  Ui::CustomResolutionDialog ui;
  ui.setupUi(&dialog);
  QIntValidator* validator = new QIntValidator(&dialog);
  validator->setBottom(1);
  ui.resolutionX->setValidator(validator);
  ui.resolutionY->setValidator(validator);
  if (dialog.exec() == QDialog::Accepted)
  {
    const int dx = ui.resolutionX->text().toInt();
    const int dy = ui.resolutionY->text().toInt();
    const QString label = ui.resolutionLabel->text();
    if (this->prependCustomResolution(dx, dy, label))
    {
      this->lockResolution(dx, dy);
    }
    // this is not needed, but just ensures that our current test playback
    // infrastructure doesn't croak.
    this->updateCustomActions();
  }
}

//-----------------------------------------------------------------------------
QAction* pqPreviewMenuManager::findAction(int dx, int dy)
{
  QString prefix = QString("%1 x %2").arg(dx).arg(dy);

  foreach (QAction* actn, this->parentMenu()->actions())
  {
    if (actn->text().startsWith(prefix))
    {
      return actn;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::lockResolution(bool lock)
{
  if (QAction* actn = qobject_cast<QAction*>(this->sender()))
  {
    if (lock)
    {
      QSize size = extractSize(actn->text());
      if (!size.isEmpty())
      {
        this->lockResolution(size.width(), size.height(), actn);
        // if `actn` is a custom action, let's sort the custom resolutions list to
        // have the most recently used item at the top of the list.
        if (actn->data().toBool())
        {
          this->prependCustomResolution(size.width(), size.height(), extractLabel(actn->text()));
          // no need to update menu. It will get updated before showing.
        }
      }
    }
    else
    {
      // unlock.
      this->unlock();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::lockResolution(int dx, int dy)
{
  if (QSize(dx, dy).isEmpty())
  {
    this->unlock();
  }
  else
  {
    this->lockResolution(dx, dy, nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::lockResolution(int dx, int dy, QAction* target)
{
  Q_UNUSED(target);

  SCOPED_UNDO_SET("Enter Preview mode");
  assert(dx >= 1 && dy >= 1);
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  assert(viewManager);

  const QSize requestedSize(dx, dy);
  const QSize previewSize = viewManager->preview(requestedSize);
  if (requestedSize != previewSize)
  {
    pqCoreUtilities::promptUser("pqPreviewMenuManager/LockResolutionPrompt",
      QMessageBox::Information, "Requested resolution too big for window",
      "The resolution requested is too big for the current window. Fitting to aspect ratio "
      "instead.",
      QMessageBox::Ok | QMessageBox::Save, pqCoreUtilities::mainWidget());
  }
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::unlock()
{
  SCOPED_UNDO_SET("Exit Preview mode");
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  assert(viewManager);
  viewManager->preview(QSize());
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::aboutToShow()
{
  this->updateCustomActions();

  auto layout = pqActiveObjects::instance().activeLayout();
  assert(layout != nullptr);
  int resolution[2];
  vtkSMPropertyHelper(layout, "PreviewMode").Get(resolution, 2);

  foreach (QAction* other, this->parentMenu()->actions())
  {
    if (other->isChecked())
    {
      other->setChecked(false);
    }
  }

  if (resolution[0] > 0 && resolution[1] > 0)
  {

    // find the corresponding item and lock it, otherwise create one
    if (QAction* actn = this->findAction(resolution[0], resolution[1]))
    {
      actn->setChecked(true);
    }
    else
    {
      // this can happen if the preview mode state is directly coming from
      // Python. In that case we add the option to the menu, but not to the
      // settings.
      actn = this->parentMenu()->addAction(generateText(resolution[0], resolution[1]));
      SETUP_ACTION(actn);
      actn->setData(true); // custom action.
      actn->setChecked(true);
    }
  }
}
