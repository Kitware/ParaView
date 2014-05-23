/*=========================================================================

   Program: ParaView
   Module:    pqStandardViewFrameActionsImplementation.cxx

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
#include "pqStandardViewFrameActionsImplementation.h"
#include "ui_pqEmptyView.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCameraUndoRedoReaction.h"
#include "pqChartSelectionReaction.h"
#include "pqContextView.h"
#include "pqCoreUtilities.h"
#include "pqDataQueryReaction.h"
#include "pqEditCameraReaction.h"
#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqRenderView.h"
#include "pqRenderViewSelectionReaction.h"
#include "pqServer.h"
#include "pqSpreadSheetViewDecorator.h"
#include "pqSpreadSheetView.h"
#include "pqToggleInteractionViewMode.h"
#include "pqUndoStack.h"
#include "pqViewFrame.h"
#include "vtkChart.h"
#include "vtkContextScene.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

#include <QMenu>
#include <QPushButton>
#include <QSet>
#include <QShortcut>

//-----------------------------------------------------------------------------
pqStandardViewFrameActionsImplementation::pqStandardViewFrameActionsImplementation(QObject* parentObject)
  : QObject(parentObject)
{
  QWidget* mainWindow = pqCoreUtilities::mainWidget();
  this->ShortCutSurfaceCells = new QShortcut(QKeySequence(tr("s")), mainWindow);
  this->ShortCutSurfacePoints = new QShortcut(QKeySequence(tr("d")), mainWindow);
  this->ShortCutFrustumCells = new QShortcut(QKeySequence(tr("f")), mainWindow);
  this->ShortCutFrustumPoints = new QShortcut(QKeySequence(tr("g")), mainWindow);
  this->ShortCutBlocks = new QShortcut(QKeySequence("b"), mainWindow);

  QObject::connect(this->ShortCutSurfaceCells, SIGNAL(activated()),
    this, SLOT(selectSurfaceCellsTrigerred()));
  QObject::connect(this->ShortCutSurfacePoints, SIGNAL(activated()),
    this, SLOT(selectSurfacePointsTrigerred()));
  QObject::connect(this->ShortCutFrustumCells, SIGNAL(activated()),
    this, SLOT(selectFrustumCellsTriggered()));
  QObject::connect(this->ShortCutFrustumPoints, SIGNAL(activated()),
    this, SLOT(selectFrustumPointsTriggered()));
  QObject::connect(this->ShortCutBlocks, SIGNAL(activated()),
    this, SLOT(selectBlocksTriggered()));
}

//-----------------------------------------------------------------------------
pqStandardViewFrameActionsImplementation::~pqStandardViewFrameActionsImplementation()
{
  delete this->ShortCutSurfaceCells;
  delete this->ShortCutSurfacePoints;
  delete this->ShortCutFrustumCells;
  delete this->ShortCutFrustumPoints;
  delete this->ShortCutBlocks;
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::frameConnected(pqViewFrame *frame, pqView *view)
{
  Q_ASSERT(frame != NULL);
  if (view == NULL)
    {
    // Setup the UI shown when no view is present in the frame.
    QWidget* empty_frame = new QWidget(frame);
    this->setupEmptyFrame(empty_frame);
    frame->setCentralWidget(empty_frame);
    }
  else
    {
    // add view-type independent actions.
    this->addGenericActions(frame, view);
    if (pqContextView* const chart_view = qobject_cast<pqContextView*>(view))
      {
      this->addContextViewActions(frame, chart_view);
      }
    else if (pqRenderView* const render_view = qobject_cast<pqRenderView*>(view))
      {
      this->addRenderViewActions(frame, render_view);
      }
    else if (pqSpreadSheetView* const sp_view = qobject_cast<pqSpreadSheetView*>(view))
      {
      this->addSpreadSheetViewActions(frame, sp_view);
      }
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::addContextViewActions(
  pqViewFrame* frame, pqContextView* chart_view)
{
  // Adding special selection controls for chart/context view
  Q_ASSERT(chart_view);
  Q_ASSERT(frame);

  QAction* toggle = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSelectChartToggle16.png"),
    "Toggle selection");
  toggle->setObjectName("actionChartToggle");
  toggle->setCheckable(true);
  toggle->setData(QVariant(vtkContextScene::SELECTION_TOGGLE));

  QAction* minus = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSelectChartMinus16.png"),
    "Subtract selection");
  minus->setObjectName("actionChartMinus");
  minus->setCheckable(true);
  minus->setData(QVariant(vtkContextScene::SELECTION_SUBTRACTION));

  QAction* plus = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSelectChartPlus16.png"),
    "Add selection");
  plus->setObjectName("actionChartPlus");
  plus->setCheckable(true);
  plus->setData(QVariant(vtkContextScene::SELECTION_ADDITION));

  QActionGroup* modeGroup = new QActionGroup(frame);
  modeGroup->addAction(plus);
  modeGroup->addAction(minus);
  modeGroup->addAction(toggle);

  /// If a QAction is added to an exclusive QActionGroup, then a checked action
  /// cannot be unchecked by clicking on it. We need that to work. Hence, we
  /// manually manage the exclusivity of the action group.
  modeGroup->setExclusive(false);
  this->QObject::connect(modeGroup, SIGNAL(triggered(QAction*)),
    SLOT(manageGroupExclusivity(QAction*)));

  QAction* chartSelectPolygonAction = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSelectChartPolygon16.png"),
    "Polygon Selection (d)");
  chartSelectPolygonAction->setObjectName("actionChartSelectPolygon");
  chartSelectPolygonAction->setCheckable(true);
  chartSelectPolygonAction->setData(QVariant(vtkChart::SELECT_POLYGON));

  QAction* chartSelectRectangularAction = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSelectChart16.png"),
    "Rectangle Selection (s)");
  chartSelectRectangularAction->setObjectName("actionChartSelectRectangular");
  chartSelectRectangularAction->setCheckable(true);
  chartSelectRectangularAction->setData(QVariant(vtkChart::SELECT_RECTANGLE));

  QActionGroup* group = new QActionGroup(frame);
  group->addAction(chartSelectPolygonAction);
  group->addAction(chartSelectRectangularAction);
  /// If a QAction is added to an exclusive QActionGroup, then a checked action
  /// cannot be unchecked by clicking on it. We need that to work. Hence, we
  /// manually manage the exclusivity of the action group.
  group->setExclusive(false);
  this->QObject::connect(group, SIGNAL(triggered(QAction*)),
    SLOT(manageGroupExclusivity(QAction*)));

  new pqChartSelectionReaction(chartSelectPolygonAction, chart_view, modeGroup);
  new pqChartSelectionReaction(chartSelectRectangularAction, chart_view, modeGroup);
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::addGenericActions(
  pqViewFrame *frame, pqView *view)
{
  Q_ASSERT(frame);
  Q_ASSERT(view);

  /// Add convert-to menu.
  frame->contextMenu()->addSeparator();
  QMenu* convertMenu = frame->contextMenu()->addMenu("Convert To ...");
  QObject::connect(convertMenu, SIGNAL(aboutToShow()),
    this, SLOT(aboutToShowConvertMenu()));

  if (view->supportsUndo())
    {
    // Setup undo/redo connections if the view module
    // supports interaction undo.
    QAction* backAction = frame->addTitleBarAction(
      QIcon(":/pqWidgets/Icons/pqUndoCamera24.png"),
      "Camera Undo");
    backAction->setObjectName("BackButton");
    new pqCameraUndoRedoReaction(backAction, true, view);

    QAction* forwardAction = frame->addTitleBarAction(
      QIcon(":/pqWidgets/Icons/pqRedoCamera24.png"),
      "Camera Redo");
    forwardAction->setObjectName("ForwardButton");
    new pqCameraUndoRedoReaction(forwardAction, false, view);
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::addRenderViewActions(
  pqViewFrame* frame, pqRenderView* renderView)
{
  Q_ASSERT(renderView);
  Q_ASSERT(frame);

  //QAction* actionPickObject = frame->addTitleBarAction(
  //  QIcon(":/pqWidgets/Icons/pqMousePick15.png"), "Pick Object (n)");
  //actionPickObject->setObjectName("actionPickObject");
  //actionPickObject->setCheckable (true);
  //actionPickObject->setShortcut(QString("n"));
  QAction* interactionModeAction = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqInteractionMode3D16.png"),
    "Change Interaction Mode");
  interactionModeAction->setObjectName("ToggleInteractionMode");
  new pqToggleInteractionViewMode(interactionModeAction, renderView);

  QAction* cameraAction = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqEditCamera16.png"), "Adjust Camera");
  cameraAction->setObjectName("CameraButton");
  new pqEditCameraReaction(cameraAction, renderView);

  QAction* actionSelectionMode = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSurfaceSelectionCell24.png"), "Select Cells On (s)");
  actionSelectionMode->setObjectName("actionSelectionMode");
  actionSelectionMode->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelectionMode, renderView,
    pqRenderViewSelectionReaction::SELECT_SURFACE_CELLS);

  QAction* actionSelectSurfacePoints = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSurfaceSelectionPoint24.png"), "Select Points On (d)");
  actionSelectSurfacePoints->setObjectName("actionSelectSurfacePoints");
  actionSelectSurfacePoints->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelectSurfacePoints, renderView,
    pqRenderViewSelectionReaction::SELECT_SURFACE_POINTS);

  QAction* actionSelect_Frustum = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqFrustumSelectionCell24.png"),
    "Select Cells Through (f)");
  actionSelect_Frustum->setObjectName("actionSelect_Frustum");
  actionSelect_Frustum->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelect_Frustum, renderView,
    pqRenderViewSelectionReaction::SELECT_FRUSTUM_CELLS);

  QAction* actionSelectFrustumPoints = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqFrustumSelectionPoint24.png"),
    "Select Points Through (g)");
  actionSelectFrustumPoints->setObjectName("actionSelectFrustumPoints");
  actionSelectFrustumPoints->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelectFrustumPoints, renderView,
    pqRenderViewSelectionReaction::SELECT_FRUSTUM_POINTS);

  QAction* actionSelectionPolygonCells = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqPolygonSelectSurfaceCell24.png"),
    "Select Cells With Polygon");
  actionSelectionPolygonCells->setObjectName("actionPolygonSelectionCells");
  actionSelectionPolygonCells->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelectionPolygonCells, renderView,
    pqRenderViewSelectionReaction::SELECT_SURFACE_CELLS_POLYGON);

  QAction* actionSelectionPolygonPoints = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqPolygonSelectSurfacePoint24.png"),
    "Select Points With Polygon");
  actionSelectionPolygonPoints->setObjectName("actionPolygonSelectionPoints");
  actionSelectionPolygonPoints->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelectionPolygonPoints, renderView,
    pqRenderViewSelectionReaction::SELECT_SURFACE_POINTS_POLYGON);

  QAction* actionSelect_Block = frame->addTitleBarAction(
    QIcon(":/pqWidgets/Icons/pqSelectBlock24.png"), "Select Block (b)");
  actionSelect_Block->setObjectName("actionSelect_Block");
  actionSelect_Block->setCheckable (true);
  new pqRenderViewSelectionReaction(actionSelect_Block, renderView,
    pqRenderViewSelectionReaction::SELECT_BLOCKS);
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::addSpreadSheetViewActions(
  pqViewFrame* frame, pqSpreadSheetView* spreadSheet)
{
  Q_ASSERT(frame);
  Q_ASSERT(spreadSheet);
  new pqSpreadSheetViewDecorator(spreadSheet);
}

//-----------------------------------------------------------------------------
// Comparator for strings with a twist. It tries to put strings with "Render
// View" at the top of the sorted list.
bool pqStandardViewFrameActionsImplementation::ViewTypeComparator(
  const pqStandardViewFrameActionsImplementation::ViewType& one,
  const pqStandardViewFrameActionsImplementation::ViewType& two)
{
  bool inone = one.Label.contains("Render View", Qt::CaseInsensitive);
  bool intwo = two.Label.contains("Render View", Qt::CaseInsensitive);

  if ((inone && intwo) || (!inone && !intwo))
    {
    return one.Label.toLower() < two.Label.toLower();
    }
  Q_ASSERT(inone || intwo);
  // one is less if it has "Render View", else two is less.
  return inone;
}

//-----------------------------------------------------------------------------
QList<pqStandardViewFrameActionsImplementation::ViewType>
pqStandardViewFrameActionsImplementation::availableViewTypes()
{
  // Iterate over all available "views".
  QList<ViewType> views;
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server) { return views; }
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("views"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMProxy* prototype = pxm->GetPrototypeProxy("views", iter->GetProxyName());
    if (prototype)
      {
      ViewType info;
      info.Label = prototype->GetXMLLabel();
      info.Name = iter->GetProxyName();
      views.push_back(info);
      }
    }
  qSort(views.begin(), views.end(),
    pqStandardViewFrameActionsImplementation::ViewTypeComparator);
  return views;
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::aboutToShowConvertMenu()
{
  QMenu* menu = qobject_cast<QMenu*>(this->sender());
  if (menu)
    {
    menu->clear();
    QList<ViewType> views = this->availableViewTypes();
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
void pqStandardViewFrameActionsImplementation::setupEmptyFrame(QWidget* frame)
{
  Ui::EmptyView ui;
  ui.setupUi(frame);

  QList<ViewType> views = this->availableViewTypes();
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
void pqStandardViewFrameActionsImplementation::invoked()
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

  BEGIN_UNDO_SET(QString("%1 %2").arg(command).arg(label));
  ViewType vtype;
  vtype.Label = label;
  vtype.Name = type;
  this->handleCreateView(vtype);
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::handleCreateView(
  const pqStandardViewFrameActionsImplementation::ViewType& viewType)
{
  pqObjectBuilder* builder =
    pqApplicationCore::instance()-> getObjectBuilder();

  // destroy active-view, if present (implying convert was called).
  if (pqActiveObjects::instance().activeView())
    {
    builder->destroy(pqActiveObjects::instance().activeView());
    }
  if (viewType.Name != "None")
    {
    builder->createView(viewType.Name, pqActiveObjects::instance().activeServer());
    }
}

//-----------------------------------------------------------------------------
namespace
{
  inline QAction* findActiveAction(const QString& name)
    {
    pqView* activeView = pqActiveObjects::instance().activeView();
    if (activeView && activeView->getWidget() &&
      activeView->getWidget()->parentWidget())
      {
      return activeView->getWidget()->parentWidget()->findChild<QAction*>(name);
      }
    return NULL;
    }

  inline void triggerAction(const QString& name)
    {
    QAction* atcn = findActiveAction(name);
    if (atcn)
      {
      atcn->trigger();
      }
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::selectSurfaceCellsTrigerred()
{
  pqView* activeView = pqActiveObjects::instance().activeView();
  pqContextView *chartView = qobject_cast<pqContextView*>(activeView);

  if(chartView)
    {
    // if we are in a chart view then trigger the chart selection
    triggerAction("actionChartSelectRectangular");
    }
  else
    {
    // else trigger the render view selection
    triggerAction("actionSelectionMode");
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::selectSurfacePointsTrigerred()
{
  pqView* activeView = pqActiveObjects::instance().activeView();
  pqContextView *chartView = qobject_cast<pqContextView*>(activeView);

  if(chartView)
    {
    // if we are in a chart view then trigger the chart selection
    triggerAction("actionChartSelectPolygon");
    }
  else
    {
    // else trigger the render view selection
    triggerAction("actionSelectSurfacePoints");
    }
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::selectFrustumCellsTriggered()
{
  triggerAction("actionSelect_Frustum");
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::selectFrustumPointsTriggered()
{
  triggerAction("actionSelectFrustumPoints");
}

//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::selectBlocksTriggered()
{
  triggerAction("actionSelect_Block");
}


//-----------------------------------------------------------------------------
void pqStandardViewFrameActionsImplementation::manageGroupExclusivity(QAction* curAction)
{
  if (!curAction || !curAction->isChecked())
    {
    return;
    }

  QActionGroup* group = qobject_cast<QActionGroup*>(this->sender());
  foreach (QAction* groupAction, group->actions())
    {
    if (groupAction != curAction && groupAction->isChecked())
      {
      groupAction->setChecked(false);
      }
    }
}
