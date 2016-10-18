#include "pqGMVReaderPanel.h"

pqGMVReaderPanel::pqGMVReaderPanel(pqProxy* prox, QWidget* par)
  : Superclass(prox, par)
{
  // Determine whether pipeline has time steps at all
  this->timeSteps =
    vtkSMIntVectorProperty::SafeDownCast(this->proxy()->GetProperty("HasProbtimeKeyword"));

  // If reader did not report any time steps, then disable the checkbox.
  if (this->timeSteps)
    if (!this->timeSteps->GetNumberOfElements() || this->timeSteps->GetElements()[0] == 0)
    {
      this->IgnoreReaderWidget = this->findChild<QCheckBox*>("IgnoreReaderTime");
      if (!this->IgnoreReaderWidget)
      {
        qWarning() << "Failed to locate Ignore Reader Time widget.";
        return;
      }
      this->IgnoreReaderWidget->setEnabled(false);
    }

  // Disable the checkbox "Import Tracers" if file(s) do not provide
  // tracers.

  // Determine the number of tracers pipeline has
  this->tracers = vtkSMIntVectorProperty::SafeDownCast(this->proxy()->GetProperty("HasTracers"));

  if (this->tracers)
  {
    this->ImportTracersWidget = this->findChild<QCheckBox*>("ImportTracers");
    if (!this->ImportTracersWidget)
    {
      qWarning() << "Failed to locate Import Tracers widget.";
      return;
    }

    // If reader did not report any tracers, then disable the checkbox.
    if (!this->tracers->GetNumberOfElements() || this->tracers->GetElements()[0] == 0)
    {
      this->ImportTracersWidget->setChecked(Qt::Unchecked);
      this->ImportTracersWidget->setEnabled(false);
    }

    // Reader did report tracers. Whenever ImportTracers checkbox is unchecked,
    // automatically uncheck all tracer point data fields. If it is checked,
    // re-check these fields again.
    else
    {
      this->treeWidget = this->findChild<pqTreeWidget*>("CellAndPointArrayStatus");

      QObject::connect(this->ImportTracersWidget, SIGNAL(stateChanged(int)), this,
        SLOT(updateTracerDataStatus(int)));
      this->ImportTracersWidget->setChecked(Qt::Checked);
      this->ImportTracersWidget->setEnabled(true);
    }
  }

  // Disable the checkbox "Import Polygons" if file(s) do not provide
  // tracers.

  // Determine the number of polygons pipeline has
  this->polygons = vtkSMIntVectorProperty::SafeDownCast(this->proxy()->GetProperty("HasPolygons"));

  if (this->polygons)
  {
    this->ImportPolygonsWidget = this->findChild<QCheckBox*>("ImportPolygons");
    if (!this->ImportPolygonsWidget)
    {
      qWarning() << "Failed to locate Import Polygons widget.";
      return;
    }

    // If reader did not report any polygons, then disable the checkbox.
    if (!this->polygons->GetNumberOfElements() || this->polygons->GetElements()[0] == 0)
    {
      this->ImportPolygonsWidget->setChecked(Qt::Unchecked);
      this->ImportPolygonsWidget->setEnabled(false);
    }
  }
}

//------------------------------------------------------------
void pqGMVReaderPanel::updateTracerDataStatus(int state)
{
  if (this->treeWidget)
  {
    bool enabled = (state == Qt::Checked);
    for (int i = 0; i < this->treeWidget->topLevelItemCount(); i++)
    {
      QTreeWidgetItem* item = this->treeWidget->topLevelItem(i);
      pqTreeWidgetItemObject* DisplItem;
      DisplItem = static_cast<pqTreeWidgetItemObject*>(item);

      if (item->data(0, Qt::DisplayRole).toString().left(7).toUpper() == "TRACER ")
      {
        DisplItem->setChecked(enabled);
      }
    }
  }
}
