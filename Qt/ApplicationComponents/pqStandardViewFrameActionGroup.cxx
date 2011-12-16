/*=========================================================================

   Program: ParaView
   Module:    pqStandardViewFrameActionGroup.cxx

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
#include "pqStandardViewFrameActionGroup.h"
#include "ui_pqEmptyView.h"

#include "pqApplicationCore.h"
#include "pqCameraUndoRedoReaction.h"
#include "pqEditCameraReaction.h"
#include "pqInterfaceTracker.h"
#include "pqMultiViewFrame.h"
#include "pqRenderView.h"
#include "pqViewModuleInterface.h"
#include "pqViewSettingsReaction.h"

#include <QMenu>
#include <QPushButton>
#include <QSet>

//-----------------------------------------------------------------------------
pqStandardViewFrameActionGroup::pqStandardViewFrameActionGroup(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqStandardViewFrameActionGroup::~pqStandardViewFrameActionGroup()
{
}

//-----------------------------------------------------------------------------
bool pqStandardViewFrameActionGroup::connect(pqMultiViewFrame *frame, pqView *view)
{
  Q_ASSERT(frame != NULL);

  frame->getContextMenu()->addSeparator();
  QMenu* convertMenu = frame->getContextMenu()->addMenu("Convert To ...");
  QObject::connect(convertMenu, SIGNAL(aboutToShow()),
    this, SLOT(aboutToShowConvertMenu()));

  if (view == NULL)
    {
    // Setup the UI shown when no view is present in the frame.
    this->setupEmptyFrame(frame->emptyMainWidget());
    return true;
    }

  pqRenderView* const render_module = qobject_cast<pqRenderView*>(view);
  if (render_module)
    {
    QAction* cameraAction = new QAction(QIcon(":/pqWidgets/Icons/pqEditCamera16.png"),
      "Adjust Camera",
      this);
    cameraAction->setObjectName("CameraButton");
    frame->addTitlebarAction(cameraAction);
    new pqEditCameraReaction(cameraAction, view);
    }

  QAction* optionsAction = new QAction(
    QIcon(":/pqWidgets/Icons/pqOptions16.png"), "Edit View Options", this);
  optionsAction->setObjectName("OptionsButton");
  frame->addTitlebarAction(optionsAction);
  new pqViewSettingsReaction(optionsAction, view);

  if (view->supportsUndo())
    {
    // Setup undo/redo connections if the view module
    // supports interaction undo.
    QAction* forwardAction = new QAction(QIcon(":/pqWidgets/Icons/pqRedoCamera24.png"),
      "Camera Redo",
      this);
    forwardAction->setObjectName("ForwardButton");
    frame->addTitlebarAction(forwardAction);
    new pqCameraUndoRedoReaction(forwardAction, false, view);

    QAction* backAction = new QAction(QIcon(":/pqWidgets/Icons/pqUndoCamera24.png"),
      "Camera Undo",
      this);
    backAction->setObjectName("BackButton");
    frame->addTitlebarAction(backAction);
    new pqCameraUndoRedoReaction(backAction, true, view);
    }
  return true;
}

//-----------------------------------------------------------------------------
inline void REMOVE_ACTION(const char* name, pqMultiViewFrame* frame)
{
  QAction* action = frame->getAction(name);
  if (action)
    {
    frame->removeTitlebarAction(action);
    delete action;
    }
}
//-----------------------------------------------------------------------------
bool pqStandardViewFrameActionGroup::disconnect(pqMultiViewFrame *frame, pqView *)
{
  REMOVE_ACTION("CameraButton", frame);
  REMOVE_ACTION("OptionsButton", frame);
  REMOVE_ACTION("ForwardButton", frame);
  REMOVE_ACTION("BackButton", frame);
  return true;
}

//-----------------------------------------------------------------------------
namespace
{
  static QMap<QString, QString> availableViewTypes()
    {
    QMap<QString, QString> views;
    pqInterfaceTracker* tracker =
      pqApplicationCore::instance()->interfaceTracker();
    foreach (pqViewModuleInterface* vi,
      tracker->interfaces<pqViewModuleInterface*>())
      {
      QStringList viewTypes = vi->viewTypes();
      for (int cc=0; cc < viewTypes.size(); cc++)
        {
        views[viewTypes[cc]] = vi->viewTypeName(viewTypes[cc]);
        }
      }
    return views;
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionGroup::aboutToShowConvertMenu()
{
  QMenu* menu = qobject_cast<QMenu*>(this->sender());
  if (menu)
    {
    menu->clear();
    QMap<QString, QString> views = availableViewTypes();
    for (QMap<QString, QString>::iterator iter = views.begin();
      iter != views.end(); ++iter)
      {
      QAction* view_action = new QAction(iter.value(), menu);
      view_action->setData(iter.key());
      menu->addAction(view_action);
      }
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionGroup::setupEmptyFrame(QWidget* frame)
{
  Ui::EmptyView ui;
  ui.setupUi(frame);

  QMap<QString, QString> views = availableViewTypes();
  for (QMap<QString, QString>::iterator iter = views.begin();
    iter != views.end(); ++iter)
    {
    QPushButton* button = new QPushButton(iter.value(), ui.ConvertActionsFrame);
    button->setObjectName(iter.value());
    button->setProperty("pqStandardViewFrameActionGroup_VIEW_TYPE", iter.key());
    ui.ConvertActionsFrame->layout()->addWidget(button);
    }
}
