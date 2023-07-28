// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqContextView.h"

#include "pqDataRepresentation.h"
#include "pqEventDispatcher.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqQVTKWidget.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqUndoStack.h"

#include "vtkAnnotationLink.h"
#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include <vtksys/SystemTools.hxx>

#include <QDebug>
#include <QList>
#include <QPointer>

#include <cassert>

// Command implementation
class pqContextView::command : public vtkCommand
{
public:
  static command* New(pqContextView& view) { return new command(view); }

  command(pqContextView& view)
    : Target(view)
  {
  }

  void Execute(vtkObject*, unsigned long, void*) override { Target.selectionChanged(); }

  pqContextView& Target;

private:
  void operator=(const pqContextView::command&);
};

class pqContextView::pqInternal
{
public:
  int SelectionAction;

  pqInternal()
    : SelectionAction(vtkChart::SELECT_RECTANGLE)
  {
  }
  ~pqInternal() = default;

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqContextView::pqContextView(const QString& type, const QString& group, const QString& name,
  vtkSMViewProxy* viewProxy, pqServer* server, QObject* parentObject)
  : Superclass(type, group, name, viewProxy, server, parentObject)
{
  this->Internal = new pqContextView::pqInternal();
  viewProxy->UpdateVTKObjects(); // this results in calling CreateVTKObjects().
  this->Command = command::New(*this);
  vtkObject::SafeDownCast(viewProxy->GetClientSideObject())
    ->AddObserver(vtkCommand::SelectionChangedEvent, this->Command);

  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::StartInteractionEvent, this, SLOT(startInteraction()));
  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::EndInteractionEvent, this, SLOT(endInteraction()));
}

//-----------------------------------------------------------------------------
pqContextView::~pqContextView()
{
  this->Command->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqContextView::createWidget()
{
  vtkSMContextViewProxy* proxy = this->getContextViewProxy();
  assert(proxy);

  // Enable multisample for chart views when not running tests. Multisamples
  // is disabled for testing to avoid failures due to antialiasing
  // differences.
  bool use_multisampling = (!vtksys::SystemTools::HasEnv("DASHBOARD_TEST_FROM_CTEST"));
  auto renWin = proxy->GetRenderWindow();
  renWin->SetMultiSamples(use_multisampling ? 8 : 0);

  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->setContextMenuPolicy(Qt::NoContextMenu);
  vtkwidget->installEventFilter(this);

  vtkwidget->setRenderWindow(proxy->GetRenderWindow());
  proxy->SetupInteractor(vtkwidget->interactor());
  return vtkwidget;
}

//-----------------------------------------------------------------------------
/// Returns the internal vtkChartView that provides the implementation for
/// the chart rendering.
vtkContextView* pqContextView::getVTKContextView() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy())->GetContextView();
}

//-----------------------------------------------------------------------------
vtkSMContextViewProxy* pqContextView::getContextViewProxy() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
bool pqContextView::supportsSelection() const
{
  return true;
}

//-----------------------------------------------------------------------------
/// Resets the zoom level to 100%.
void pqContextView::resetDisplay(bool vtkNotUsed(closest))
{
  vtkSMContextViewProxy* proxy = this->getContextViewProxy();
  if (proxy)
  {
    proxy->ResetDisplay();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqContextView::selectionChanged()
{
  // Fill the selection source with the selection from the view
  vtkSelection* sel = this->getContextViewProxy()->GetCurrentSelection();
  if (sel)
  {
    this->setSelection(sel);
  }
}

//-----------------------------------------------------------------------------
void pqContextView::setSelection(vtkSelection* sel)
{
  // Get the representation's source
  pqDataRepresentation* pqRepr = nullptr;

  for (int i = 0; i < this->getNumberOfRepresentations(); ++i)
  {
    if (this->getRepresentation(i)->isVisible())
    {
      pqRepr = qobject_cast<pqDataRepresentation*>(this->getRepresentation(i));
    }
  }

  if (!pqRepr)
  {
    return;
  }

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(opPort->getSource()->getProxy());

  repSource->CleanSelectionInputs(opPort->getPortNumber());

  vtkSmartPointer<vtkSMSourceProxy> selectionSource;
  selectionSource.TakeReference(vtkSMSourceProxy::SafeDownCast(
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(repSource->GetSession(), sel)));

  // If not selection has been made,
  // the selection source can be null.
  if (selectionSource)
  {
    // create a new append Selections filter and append the selection source
    vtkSmartPointer<vtkSMSourceProxy> appendSelections;
    appendSelections.TakeReference(vtkSMSourceProxy::SafeDownCast(
      vtkSMSelectionHelper::NewAppendSelectionsFromSelectionSource(selectionSource)));
    // Set the selection on the representation's source
    repSource->SetSelectionInput(opPort->getPortNumber(), appendSelections, 0);

    // Trace the selection
    if (strcmp(selectionSource->GetXMLName(), "ThresholdSelectionSource") == 0)
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("SelectThresholds")
        .arg("Thresholds", vtkSMPropertyHelper(selectionSource, "Thresholds").GetDoubleArray())
        .arg("ArrayName", vtkSMPropertyHelper(selectionSource, "ArrayName").GetAsString())
        .arg("FieldType", vtkSMPropertyHelper(selectionSource, "FieldType").GetAsInt());
    }
    else
    {
      // Map from selection source proxy name to trace function
      std::string functionName(selectionSource->GetXMLName());
      functionName.erase(functionName.size() - sizeof("SelectionSource") + 1);
      functionName.append("s");
      functionName.insert(0, "Select");

      SM_SCOPED_TRACE(CallFunction)
        .arg(functionName.c_str())
        .arg("IDs", vtkSMPropertyHelper(selectionSource, "IDs").GetIntArray())
        .arg("FieldType", vtkSMPropertyHelper(selectionSource, "FieldType").GetAsInt())
        .arg("ContainingCells", vtkSMPropertyHelper(selectionSource, "ContainingCells").GetAsInt());
    }
  }

  Q_EMIT this->selected(opPort);
}

//-----------------------------------------------------------------------------
void pqContextView::setSelectionAction(int selAction)
{
  if (this->Internal->SelectionAction == selAction || selAction < vtkChart::SELECT ||
    selAction > vtkChart::SELECT_POLYGON)
  {
    return;
  }
  this->Internal->SelectionAction = selAction;
}

//-----------------------------------------------------------------------------
int pqContextView::selectionAction()
{
  return this->Internal->SelectionAction;
}

//-----------------------------------------------------------------------------
void pqContextView::startInteraction()
{
  BEGIN_UNDO_SET(tr("Interaction"));
}

//-----------------------------------------------------------------------------
void pqContextView::endInteraction()
{
  END_UNDO_SET();
}
