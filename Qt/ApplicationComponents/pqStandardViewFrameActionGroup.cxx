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

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCameraUndoRedoReaction.h"
#include "pqChartSelectionReaction.h"
#include "pqContextView.h"
#include "pqEditCameraReaction.h"
#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqRenderView.h"
#include "pqToggleInteractionViewMode.h"
#include "pqUndoStack.h"
#include "pqViewFrame.h"
#include "pqViewModuleInterface.h"
#include "pqViewSettingsReaction.h"

#include "vtkContextScene.h"
#include "vtkChart.h"

#include <QMenu>
#include <QPushButton>
#include <QSet>

//-----------------------------------------------------------------------------
inline QAction* createChartSelectionAction(
  const QString &filename, const QString &actText,
  const QString &objName, QObject* actParent,
  int selType, pqViewFrame* actFrame, pqContextView* chart_view,
  int enumSelAction=0)
{
  QAction* selAction = new QAction(
    QIcon(filename), actText, actParent);
  selAction->setObjectName(objName);
  selAction->setCheckable(true);
  actFrame->addTitleBarAction(selAction);
  new pqChartSelectionReaction(selAction, chart_view,selType, enumSelAction);
  return selAction;
}

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
bool pqStandardViewFrameActionGroup::connect(pqViewFrame *frame, pqView *view)
{
  Q_ASSERT(frame != NULL);

  frame->contextMenu()->addSeparator();
  QMenu* convertMenu = frame->contextMenu()->addMenu("Convert To ...");
  QObject::connect(convertMenu, SIGNAL(aboutToShow()),
    this, SLOT(aboutToShowConvertMenu()));

  if (view == NULL)
    {
    // Setup the UI shown when no view is present in the frame.
    QWidget* empty_frame = new QWidget(frame);
    this->setupEmptyFrame(empty_frame);
    frame->setCentralWidget(empty_frame);
    return true;
    }

  pqRenderView* const render_module = qobject_cast<pqRenderView*>(view);
  if (render_module)
    {
    QAction* cameraAction = frame->addTitleBarAction(
      QIcon(":/pqWidgets/Icons/pqEditCamera16.png"), "Adjust Camera");
    cameraAction->setObjectName("CameraButton");
    new pqEditCameraReaction(cameraAction, view);

    QAction* interactionModeAction = frame->addTitleBarAction("3D");
    interactionModeAction->setObjectName("ToggleInteractionMode");
    new pqToggleInteractionViewMode(interactionModeAction, view);
    }

  QAction* optionsAction = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqOptions16.png"), "Edit View Options");
  optionsAction->setObjectName("OptionsButton");
  new pqViewSettingsReaction(optionsAction, view);

  if (view->supportsUndo())
    {
    // Setup undo/redo connections if the view module
    // supports interaction undo.
    QAction* forwardAction = frame->addTitleBarAction(
      QIcon(":/pqWidgets/Icons/pqRedoCamera24.png"),
      "Camera Redo");
    forwardAction->setObjectName("ForwardButton");
    new pqCameraUndoRedoReaction(forwardAction, false, view);

    QAction* backAction = frame->addTitleBarAction(
      QIcon(":/pqWidgets/Icons/pqUndoCamera24.png"),
      "Camera Undo");
    backAction->setObjectName("BackButton");
    new pqCameraUndoRedoReaction(backAction, true, view);
    }
  // Adding special selection controls for chart/context view
  pqContextView* const chart_view = qobject_cast<pqContextView*>(view);
  if (chart_view && chart_view->supportsSelection())
    {
    // selection mode
    createChartSelectionAction(
      ":/pqWidgets/Icons/pqSelectChartToggle16.png",
      "Toggle Selection (Ctrl+Shift Keys)", "ChartSelectToggleButton",
      this, vtkContextScene::SELECTION_TOGGLE, frame, chart_view);
    createChartSelectionAction(
      ":/pqWidgets/Icons/pqSelectChartMinus16.png",
      "Subtract Selection (Ctrl Key)", "ChartSelectMinusButton",
      this, vtkContextScene::SELECTION_SUBTRACTION, frame, chart_view);
    QAction* plusAction = createChartSelectionAction(
      ":/pqWidgets/Icons/pqSelectChartPlus16.png",
      "Add Selection (Shift Key)", "ChartSelectPlusButton",
      this, vtkContextScene::SELECTION_ADDITION, frame, chart_view);

    // selection action
    QAction* polySelAction = createChartSelectionAction(
      ":/pqWidgets/Icons/pqSelectChartPolygon16.png",
      "Polygon Selection", "ChartSelectPolygonButton",
      this, vtkContextScene::SELECTION_NONE, frame, chart_view,
      vtkChart::SELECT_POLYGON);
    QAction* rectSelAction = createChartSelectionAction(
      ":/pqWidgets/Icons/pqSelectChart16.png",
      "Rectangle Selection", "ChartSelectButton",
      this, vtkContextScene::SELECTION_NONE, frame, chart_view,
      vtkChart::SELECT_RECTANGLE);
    QActionGroup *selActionGroup = new QActionGroup(this);
    selActionGroup->setExclusive(true);
    selActionGroup->addAction(polySelAction);
    selActionGroup->addAction(rectSelAction);
    // by default, the rectangle selection is checked
    rectSelAction->setChecked(true);
    // The separator does not seems to show ??
    frame->contextMenu()->insertSeparator(plusAction);
    }
  return true;
}

//-----------------------------------------------------------------------------
bool pqStandardViewFrameActionGroup::disconnect(pqViewFrame *frame, pqView *)
{
  frame->removeTitleBarActions(); 
  return true;
}

//-----------------------------------------------------------------------------
namespace
{
  struct ViewType
    {
    QString Label;
    QString Name;
    };

  bool ViewTypeComparator(const ViewType& one, const ViewType& two)
    {
    return one.Label.toLower() < two.Label.toLower();
    }

  static QList<ViewType> availableViewTypes()
    {
    QList<ViewType> views;
    pqInterfaceTracker* tracker =
      pqApplicationCore::instance()->interfaceTracker();
    foreach (pqViewModuleInterface* vi,
      tracker->interfaces<pqViewModuleInterface*>())
      {
      QStringList viewTypes = vi->viewTypes();
      for (int cc=0; cc < viewTypes.size(); cc++)
        {
        ViewType info;
        info.Label = vi->viewTypeName(viewTypes[cc]);
        info.Name = viewTypes[cc];
        views.push_back(info);
        }
      }
    qSort(views.begin(), views.end(), ViewTypeComparator);
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
    QList<ViewType> views = availableViewTypes();
    foreach (const ViewType& type, views)
      {
      QAction* view_action = new QAction(type.Label, menu);
      view_action->setProperty("PV_VIEW_TYPE", type.Name);
      view_action->setProperty("PV_VIEW_LABEL", type.Label);
      view_action->setProperty("PV_COMMAND", "Convert To");
      menu->addAction(view_action);
      QObject::connect(view_action, SIGNAL(triggered()),
        this, SLOT(invoked()), Qt::QueuedConnection);
      }
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionGroup::setupEmptyFrame(QWidget* frame)
{
  Ui::EmptyView ui;
  ui.setupUi(frame);

  QList<ViewType> views = availableViewTypes();
  foreach (const ViewType& type, views)
    {
    QPushButton* button = new QPushButton(type.Label, ui.ConvertActionsFrame);
    button->setObjectName(type.Name);
    button->setProperty("PV_VIEW_TYPE", type.Name);
    button->setProperty("PV_VIEW_LABEL", type.Label);
    button->setProperty("PV_COMMAND", "Create");

    QObject::connect(button, SIGNAL(clicked()),
      this, SLOT(invoked()), Qt::QueuedConnection);
    ui.ConvertActionsFrame->layout()->addWidget(button);
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionGroup::invoked()
{
  QObject* osender = this->sender();
  if (!osender)
    {
    return;
    }

  // either create a new view, or convert the existing one.
  // This slot is called either from an action in the "Convert To" menu, or from
  // the buttons on an empty frame.
  QString type = osender->property("PV_VIEW_TYPE").toString();
  QString label = osender->property("PV_VIEW_LABEL").toString();
  QString command = osender->property("PV_COMMAND").toString();

  pqObjectBuilder* builder =
    pqApplicationCore::instance()-> getObjectBuilder();

  BEGIN_UNDO_SET(QString("%1 %2").arg(command).arg(label));

  // destroy active-view, if present (implying convert was called).
  if (pqActiveObjects::instance().activeView())
    {
    builder->destroy(pqActiveObjects::instance().activeView());
    }

  if (type != "None")
    {
    builder->createView(type, pqActiveObjects::instance().activeServer());
    }

  END_UNDO_SET();
}
