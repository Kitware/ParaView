/*=========================================================================

    Program: ParaView
    Module:    $RCSfile$

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
#include "pqIntegrationModelSurfaceHelperWidget.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDataObjectTypes.h"
#include "vtkLagrangianSurfaceHelper.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringArray.h"

#include "pqExpandableTableView.h"
#include "pqQtDeprecated.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QPushButton>
#include <QStandardItemModel>

#include <sstream>

//-----------------------------------------------------------------------------
pqIntegrationModelSurfaceHelperWidget::pqIntegrationModelSurfaceHelperWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, smproperty, parentObject)
{
  this->resetSurfaceWidget(true);
  this->addPropertyLink(this, "arrayToGenerate", SIGNAL(arrayToGenerateChanged()), smproperty);
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSurfaceHelperWidget::resetWidget()
{
  this->resetSurfaceWidget(false);
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSurfaceHelperWidget::resetSurfaceWidget(bool force)
{
  // Avoid resetting when it is not necessary
  if (force || this->ModelProperty->GetUncheckedProxy(0) != this->ModelPropertyValue)
  {
    // Recover the current unchecked value, ie the value selected by the user but not yet applied
    this->ModelPropertyValue = this->ModelProperty->GetUncheckedProxy(0);

    // Remove all previous layout and child widget
    qDeleteAll(this->children());

    if (this->ModelPropertyValue)
    {
      // Force push the model proxy property to the server
      this->ModelProperty->SetProxy(0, this->ModelPropertyValue);
      this->proxy()->UpdateProperty("IntegrationModel", true);

      // Rexecute RequestInformation pass for the surface helper to update the surface datasets
      vtkSMSourceProxy::SafeDownCast(this->proxy())->UpdatePipelineInformation();

      // Update surface default values
      this->ModelPropertyValue->UpdatePropertyInformation();

      // Recover model array names and components
      vtkSMStringVectorProperty* namesProp = vtkSMStringVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SurfaceArrayNames"));
      vtkSMIntVectorProperty* compsProp = vtkSMIntVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SurfaceArrayComps"));
      vtkSMStringVectorProperty* enumsProp = vtkSMStringVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SurfaceArrayEnumValues"));
      vtkSMDoubleVectorProperty* defaultValuesProp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SurfaceArrayDefaultValues"));
      vtkSMIntVectorProperty* typesProp = vtkSMIntVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SurfaceArrayTypes"));

      // Recover input and associated flat leaf names
      vtkSMInputProperty* inputProperty =
        vtkSMInputProperty::SafeDownCast(this->proxy()->GetProperty("SurfaceInput"));
      if (!inputProperty)
      {
        qCritical() << "No Input Property defined, cannot reset widget";
        return;
      }
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputProperty->GetProxy(0));
      if (!input)
      {
        qCritical() << "No Input defined, cannot reset widget";
        return;
      }

      vtkNew<vtkStringArray> leafNames;
      pqIntegrationModelSurfaceHelperWidget::fillLeafNames(
        input->GetDataInformation(), "", leafNames.Get());
      unsigned int nLeafs = leafNames->GetNumberOfValues();

      /// Check number of components
      unsigned int nArrays = namesProp->GetNumberOfElements();
      if (nArrays != compsProp->GetNumberOfElements())
      {
        qCritical() << "not the same number of names and components: "
                    << namesProp->GetNumberOfElements()
                    << " != " << compsProp->GetNumberOfElements()
                    << ", array generation may be invalid";
      }

      // Create main grid layout
      QGridLayout* gridLayout = new QGridLayout(this);

      unsigned int enumIndex = 0;
      unsigned int defaultValuesIndex = 0;
      for (unsigned int i = 0; i < nArrays; i++)
      {
        const char* arrayName = namesProp->GetElement(i);
        const char* labelName = vtkSMProperty::CreateNewPrettyLabel(arrayName);
        int nComponents = compsProp->GetElement(i);
        int type = typesProp->GetElement(i);

        // Create a group box for each array
        QGroupBox* gb = new QGroupBox(labelName, this);
        gb->setCheckable(true);
        gb->setChecked(false);
        gb->setProperty("name", arrayName);
        gb->setProperty("type", type);
        QObject::connect(gb, SIGNAL(toggled(bool)), this, SIGNAL(arrayToGenerateChanged()));
        delete[] labelName;

        // And associated layout
        QGridLayout* gbLayout = new QGridLayout(gb);
        gb->setLayout(gbLayout);

        // Create a pqExpandableView in groupbox
        pqExpandableTableView* table = new pqExpandableTableView(this);
        QStandardItemModel* model = new QStandardItemModel(nLeafs, nComponents);
        table->setModel(model);
        table->horizontalHeader()->setHighlightSections(false);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->hide();
        QObject::connect(
          model, SIGNAL(itemChanged(QStandardItem*)), this, SIGNAL(arrayToGenerateChanged()));

        // Enum if necessary
        int nbOfEnum = QString(enumsProp->GetElement(enumIndex)).toInt();
        enumIndex++;

        // Add a row for each leaf
        for (unsigned int j = 0; j < nLeafs; j++)
        {
          QStandardItem* item;
          item = new QStandardItem(QString(leafNames->GetValue(j)));
          item->setFlags(item->flags() & ~Qt::ItemIsEditable); // non editable
          model->setVerticalHeaderItem(j, item);

          // Add a column for each components
          for (int k = 0; k < nComponents; k++)
          {
            item = new QStandardItem();
            model->setItem(j, k, item);
            double defaultValue = 0;
            if (defaultValuesIndex < defaultValuesProp->GetNumberOfElements())
            {
              defaultValue = defaultValuesProp->GetElement(defaultValuesIndex);
              defaultValuesIndex++;
            }
            if (nbOfEnum > 0)
            {
              QModelIndex modelIndex = model->indexFromItem(item);
              QComboBox* cmb = new QComboBox;
              QObject::connect(
                cmb, SIGNAL(currentIndexChanged(int)), this, SIGNAL(arrayToGenerateChanged()));
              cmb->addItem("None", "None");
              int defaultIndex = 0;
              for (int enumIt = 0; enumIt < nbOfEnum; enumIt++)
              {
                int enumVal = QString(enumsProp->GetElement(enumIndex + 2 * enumIt)).toInt();
                QString enumString = QString(enumsProp->GetElement(enumIndex + 2 * enumIt + 1));
                cmb->addItem(enumString, enumVal);
                if (enumVal == defaultValue)
                {
                  defaultIndex = enumIt + 1;
                }
              }
              cmb->setCurrentIndex(defaultIndex);
              table->setIndexWidget(modelIndex, cmb);
            }
            else
            {
              item->setText(QString::number(defaultValue));
            }
          }
        }
        enumIndex += 2 * nbOfEnum;
        gbLayout->addWidget(table, 0, 0);
        gridLayout->addWidget(gb, i, 0);
      }
    }
    Q_EMIT(this->arrayToGenerateChanged());
  }
}

namespace
{
// This relies on the internals of how  hierarchy is built in
// vtkDataAssemblyUtilities -- something that I do not like. However, this needs
// more work to cleanup and hence taking this shortcut for now.
class LeafVistor : public vtkDataAssemblyVisitor
{
public:
  static LeafVistor* New();
  vtkTypeMacro(LeafVistor, vtkDataAssemblyVisitor);

  void Visit(int nodeid) override
  {
    auto assembly = this->GetAssembly();
    auto vtk_type = assembly->GetAttributeOrDefault(nodeid, "vtk_type", VTK_DATA_OBJECT);
    if (!vtkDataObjectTypes::TypeIdIsA(vtk_type, VTK_COMPOSITE_DATA_SET) ||
      vtkDataObjectTypes::TypeIdIsA(vtk_type, VTK_MULTIPIECE_DATA_SET))
    {
      // this is a leaf node; add it's name.
      this->AppendLeaf(nodeid);
    }
  }

  void BeginSubTree(int nodeid) override
  {
    auto assembly = this->GetAssembly();
    const char* label =
      nodeid == 0 ? nullptr : assembly->GetAttributeOrDefault(nodeid, "label", nullptr);
    this->Labels.push_back(label ? std::string(label) : std::string());
  }

  void EndSubTree(int) override { this->Labels.pop_back(); }

  void GetLeaves(vtkStringArray* names) const
  {
    for (const auto& leaf : this->Leaves)
    {
      names->InsertNextValue(leaf.c_str());
    }
  }

protected:
  LeafVistor() = default;

  void AppendLeaf(int nodeid)
  {
    const int index = static_cast<int>(this->Leaves.size());
    std::ostringstream stream;
    stream << index << ":";
    for (const auto& name : this->Labels)
    {
      stream << name;
      if (!name.empty())
      {
        stream << "/";
      }
    }

    auto assembly = this->GetAssembly();
    stream << assembly->GetAttributeOrDefault(nodeid, "label", "");
    this->Leaves.push_back(stream.str());
  }

private:
  LeafVistor(const LeafVistor&) = delete;
  void operator=(const LeafVistor&) = delete;

  std::vector<std::string> Labels;
  std::vector<std::string> Leaves;
};

vtkStandardNewMacro(LeafVistor);

} // end of namespace

//-----------------------------------------------------------------------------
vtkStringArray* pqIntegrationModelSurfaceHelperWidget::fillLeafNames(
  vtkPVDataInformation* info, QString baseName, vtkStringArray* names)
{
  auto hierarchy = info->GetHierarchy();
  if (!info->IsCompositeDataSet() || hierarchy == nullptr)
  {
    names->InsertNextValue("Single DataSet");
  }
  else
  {
    vtkNew<LeafVistor> visitor;
    hierarchy->Visit(visitor, /*traversal_order=*/vtkDataAssembly::TraversalOrder::DepthFirst);
    visitor->GetLeaves(names);
  }

  return names;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqIntegrationModelSurfaceHelperWidget::arrayToGenerate() const
{
  QList<QVariant> values;
  // For each group box
  foreach (QGroupBox* gb, this->findChildren<QGroupBox*>())
  {
    if (gb->isChecked())
    {
      QTableView* table = gb->findChild<QTableView*>();
      if (table)
      {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(table->model());
        if (model)
        {
          unsigned int nLeafs = model->rowCount();
          unsigned int nComponents = model->columnCount();

          // Serial Fill up of the property
          values.push_back(gb->property("name"));
          values.push_back(gb->property("type"));
          values.push_back(nLeafs);
          values.push_back(nComponents);
          std::ostringstream str;
          for (unsigned int i = 0; i < nLeafs; i++)
          {
            for (unsigned int j = 0; j < nComponents; j++)
            {
              QComboBox* cmb = qobject_cast<QComboBox*>(table->indexWidget(model->index(i, j)));
              if (cmb)
              {
                str << cmb->itemData(cmb->currentIndex()).toString().toLatin1().data() << ";";
              }
              else
              {
                str << model->item(i, j)->text().toLatin1().data() << ";";
              }
            }
          }
          values.push_back(str.str().c_str());
        }
      }
    }
  }
  return values;
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSurfaceHelperWidget::setArrayToGenerate(const QList<QVariant>& values)
{
  QGroupBox* gb;
  QList<QGroupBox*> gbs = this->findChildren<QGroupBox*>();
  foreach (gb, gbs)
  {
    gb->setChecked(false);
  }

  for (QList<QVariant>::const_iterator i = values.begin(); i != values.end(); i += 5)
  {
    // Incremental python filling value check
    // When using python, this method is called incrementally,
    // this allows to wait for the correct call
    if ((i + 4)->toString().isEmpty())
    {
      continue;
    }

    QString arrayName = i->toString();
    int type = (i + 1)->toInt();
    gb = nullptr;
    foreach (gb, gbs)
    {
      if (arrayName == gb->property("name"))
      {
        break;
      }
    }
    if (!gb)
    {
      qCritical() << "Could not find group box with name" << arrayName;
      continue;
    }
    if (gb->property("type") != type)
    {
      qCritical() << "Dynamic array typing is not supported, type is ignored"
                  << gb->property("type") << " " << type;
    }
    QTableView* table = gb->findChild<QTableView*>();
    if (!table)
    {
      continue;
    }
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(table->model());
    if (!model)
    {
      continue;
    }
    int nLeafs = (i + 2)->toInt();
    int nComponents = (i + 3)->toInt();
    QStringList dataValues = (i + 4)->toString().split(';', PV_QT_SKIP_EMPTY_PARTS);
    if (dataValues.size() != nLeafs * nComponents)
    {
      qCritical() << "Unexpected number of values" << dataValues.size() << " "
                  << nLeafs * nComponents;
      continue;
    }
    gb->setChecked(true);
    for (int j = 0; j < nLeafs; j++)
    {
      for (int k = 0; k < nComponents; k++)
      {
        QComboBox* cmb = qobject_cast<QComboBox*>(table->indexWidget(model->index(j, k)));
        if (cmb)
        {
          cmb->setCurrentIndex(cmb->findData(dataValues[j * nComponents + k]));
        }
        else
        {
          model->item(j, k)->setText(dataValues[j * nComponents + k]);
        }
      }
    }
  }
}
