/*=========================================================================

   Program: ParaView
   Module:  pqSelectionInputWidget.cxx

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
#include "pqSelectionInputWidget.h"
#include "ui_pqSelectionInputWidget.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqSelectionManager.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <QTextStream>
#include <QtDebug>
#include <string>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#define QT_ENDL endl
#else
#define QT_ENDL Qt::endl
#endif

class pqSelectionInputWidget::pqUi : public Ui::pqSelectionInputWidget
{
};

//-----------------------------------------------------------------------------
pqSelectionInputWidget::pqSelectionInputWidget(QWidget* _parent)
  : QWidget(_parent)
{
  this->Ui = new pqSelectionInputWidget::pqUi();
  this->Ui->setupUi(this);

  QObject::connect(
    this->Ui->pushButtonCopySelection, SIGNAL(clicked()), this, SLOT(copyActiveSelection()));

  pqSelectionManager* selMan =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));

  if (selMan)
  {
    QObject::connect(
      selMan, SIGNAL(selectionChanged(pqOutputPort*)), this, SLOT(onActiveSelectionChanged()));
  }
}

//-----------------------------------------------------------------------------
pqSelectionInputWidget::~pqSelectionInputWidget()
{
  delete this->Ui;
}

//-----------------------------------------------------------------------------
void pqSelectionInputWidget::updateLabels()
{
  if (!this->AppendSelections)
  {
    this->Ui->label->setText("No selection");
    this->Ui->textBrowser->setText("");
    return;
  }

  this->Ui->label->setText("Copied Selection");

  // the selection source should be an appendSelections
  auto& appendSelections = this->AppendSelections;
  unsigned int numInputs = appendSelections && appendSelections->GetProperty("Input")
    ? vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements()
    : 0;
  QString text;
  QTextStream columnValues(&text, QIODevice::ReadWrite);
  if (numInputs > 0)
  {
    columnValues << "Number of Selections: " << numInputs << QT_ENDL;
    columnValues << "Selection Expression: "
                 << vtkSMPropertyHelper(appendSelections, "Expression").GetAsString() << QT_ENDL;
    columnValues << "Invert Selection: "
                 << (vtkSMPropertyHelper(appendSelections, "InsideOut").GetAsInt() ? "Yes" : "No")
                 << QT_ENDL;
    // using the first selection source is sufficient because all the selections source
    // will have the same fieldType/ElementType
    auto firstSelectionSource = vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy();
    QString fieldTypeAsString("Unknown");
    if (auto smproperty = firstSelectionSource->GetProperty("FieldType"))
    {
      fieldTypeAsString = vtkSMFieldDataDomain::GetElementTypeAsString(
        vtkSelectionNode::ConvertSelectionFieldToAttributeType(
          vtkSMPropertyHelper(smproperty).GetAsInt()));
    }
    else if ((smproperty = firstSelectionSource->GetProperty("ElementType")))
    {
      fieldTypeAsString =
        vtkSMFieldDataDomain::GetElementTypeAsString(vtkSMPropertyHelper(smproperty).GetAsInt());
    }
    columnValues << "Elements: " << fieldTypeAsString << QT_ENDL;
    for (unsigned int i = 0; i < numInputs; ++i)
    {
      auto selectionSource = vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(i);
      auto selectionName = vtkSMPropertyHelper(appendSelections, "SelectionNames").GetAsString(i);
      const auto proxyXMLName = selectionSource->GetXMLName();
      columnValues << QT_ENDL << selectionName << ")" << QT_ENDL << "Type: ";
      if (strcmp(proxyXMLName, "FrustumSelectionSource") == 0)
      {
        columnValues << "Frustum Selection" << QT_ENDL << QT_ENDL << "Values:" << QT_ENDL
                     << QT_ENDL;
        vtkSMProperty* dvp = selectionSource->GetProperty("Frustum");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(dvp);
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 4 != 3)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "ValueSelectionSource") == 0)
      {
        columnValues << "Values Selection" << QT_ENDL << QT_ENDL << "Array Name: "
                     << vtkSMPropertyHelper(selectionSource, "ArrayName").GetAsString() << QT_ENDL
                     << QT_ENDL << "Values" << QT_ENDL << QT_ENDL;
        vtkSMProperty* dvp = selectionSource->GetProperty("Values");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(dvp);
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 2 != 1)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "PedigreeIDSelectionSource") == 0)
      {
        columnValues << "Pedigree ID Selection" << QT_ENDL << QT_ENDL;
        vtkSMProperty* dsivp = selectionSource->GetProperty("StringIDs");
        QList<QVariant> stringIdValues = pqSMAdaptor::getMultipleElementProperty(dsivp);
        if (!stringIdValues.empty())
        {
          columnValues << "Pedigree Domain"
                       << "\t"
                       << "String ID" << QT_ENDL << QT_ENDL;
          for (int cc = 0; cc < stringIdValues.size(); cc++)
          {
            if (cc % 2 != 1)
            {
              columnValues << stringIdValues[cc].toString() << "\t";
            }
            else
            {
              columnValues << stringIdValues[cc].toString() << QT_ENDL;
            }
          }
          columnValues << QT_ENDL;
        }
        vtkSMProperty* divp = selectionSource->GetProperty("IDs");
        QList<QVariant> IdValues = pqSMAdaptor::getMultipleElementProperty(divp);
        if (!IdValues.empty())
        {
          columnValues << "Pedigree Domain"
                       << "\t"
                       << "ID" << QT_ENDL << QT_ENDL;
          for (int cc = 0; cc < IdValues.size(); cc++)
          {
            if (cc % 2 != 1)
            {
              columnValues << IdValues[cc].toString() << "\t";
            }
            else
            {
              columnValues << IdValues[cc].toString() << QT_ENDL;
            }
          }
          columnValues << QT_ENDL;
        }
      }
      else if (strcmp(proxyXMLName, "GlobalIDSelectionSource") == 0)
      {
        columnValues << "Global ID Selection" << QT_ENDL << QT_ENDL << "Global ID" << QT_ENDL
                     << QT_ENDL;
        QList<QVariant> value =
          pqSMAdaptor::getMultipleElementProperty(selectionSource->GetProperty("IDs"));
        Q_FOREACH (QVariant val, value)
        {
          columnValues << val.toString() << QT_ENDL;
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "IDSelectionSource") == 0)
      {
        columnValues << "ID Selection" << QT_ENDL << QT_ENDL << "Process ID"
                     << "\t"
                     << "Index" << QT_ENDL << QT_ENDL;
        vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(idvp);
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 2 != 1)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "CompositeDataIDSelectionSource") == 0)
      {
        columnValues << "ID Selection" << QT_ENDL << QT_ENDL << "Composite ID"
                     << "\t"
                     << "Process ID"
                     << "\t"
                     << "Index" << QT_ENDL << QT_ENDL;
        vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(idvp);
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 3 != 2)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "HierarchicalDataIDSelectionSource") == 0)
      {
        columnValues << "ID Selection" << QT_ENDL << QT_ENDL;
        columnValues << "Level"
                     << "\t"
                     << "Dataset"
                     << "\t"
                     << "Index" << QT_ENDL << QT_ENDL;
        vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(idvp);
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 3 != 2)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "LocationSelectionSource") == 0)
      {
        columnValues << "Location-based Selection" << QT_ENDL << QT_ENDL << "Probe Locations"
                     << QT_ENDL << QT_ENDL;
        vtkSMProperty* idvp = selectionSource->GetProperty("Locations");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(idvp);
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 3 != 2)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "BlockSelectionSource") == 0)
      {
        columnValues << "Block Selection" << QT_ENDL << QT_ENDL << "Blocks" << QT_ENDL << QT_ENDL;
        vtkSMProperty* prop = selectionSource->GetProperty("Blocks");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
        Q_FOREACH (const QVariant& value, values)
        {
          columnValues << value.toString() << QT_ENDL;
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "BlockSelectorsSelectionSource") == 0)
      {
        columnValues
          << "Block Selectors Selection" << QT_ENDL << QT_ENDL << "Assembly Name: "
          << vtkSMPropertyHelper(selectionSource, "BlockSelectorsAssemblyName").GetAsString()
          << QT_ENDL << QT_ENDL << "Block Selectors" << QT_ENDL << QT_ENDL;
        vtkSMProperty* prop = selectionSource->GetProperty("BlockSelectors");
        QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
        Q_FOREACH (const QVariant& value, values)
        {
          columnValues << value.toString() << QT_ENDL;
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "ThresholdSelectionSource") == 0)
      {
        columnValues << "Threshold Selection" << QT_ENDL << QT_ENDL << "Array Name: "
                     << vtkSMPropertyHelper(selectionSource, "ArrayName").GetAsString() << QT_ENDL
                     << QT_ENDL << "Thresholds" << QT_ENDL << QT_ENDL;
        QList<QVariant> values =
          pqSMAdaptor::getMultipleElementProperty(selectionSource->GetProperty("Thresholds"));
        for (int cc = 0; cc < values.size(); cc++)
        {
          if (cc % 2 != 1)
          {
            columnValues << values[cc].toString() << "\t";
          }
          else
          {
            columnValues << values[cc].toString() << QT_ENDL;
          }
        }
        columnValues << QT_ENDL;
      }
      else if (strcmp(proxyXMLName, "SelectionQuerySource") == 0)
      {
        columnValues << "Query Selection" << QT_ENDL << QT_ENDL << "Query: "
                     << vtkSMPropertyHelper(selectionSource, "QueryString").GetAsString()
                     << QT_ENDL;
      }
      else
      {
        columnValues << "None" << QT_ENDL;
      }
    }
  }
  else
  {
    columnValues << "Number of Selections: 0" << QT_ENDL;
  }

  this->Ui->textBrowser->setText(text);
  columnValues.flush();
}

//-----------------------------------------------------------------------------
// This will update the UnacceptedSelectionSource proxy with a clone of the
// active selection proxy. The filter proxy is not modified yet.
void pqSelectionInputWidget::copyActiveSelection()
{
  auto selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));

  if (!selectionManager)
  {
    qDebug() << "No selection manager was detected. "
                "Don't know where to get the active selection from.";
    return;
  }

  pqOutputPort* port = selectionManager->getSelectedPort();
  if (!port)
  {
    this->setSelection(nullptr);
    return;
  }

  vtkSMProxy* activeSelection = port->getSelectionInput();
  if (!activeSelection)
  {
    this->setSelection(nullptr);
    return;
  }

  vtkSMSessionProxyManager* pxm = activeSelection->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> newSource;
  newSource.TakeReference(
    pxm->NewProxy(activeSelection->GetXMLGroup(), activeSelection->GetXMLName()));
  newSource->Copy(activeSelection);
  newSource->UpdateVTKObjects();
  this->setSelection(newSource);
}

//-----------------------------------------------------------------------------
void pqSelectionInputWidget::setSelection(pqSMProxy newAppendSelections)
{
  if (this->AppendSelections == newAppendSelections)
  {
    return;
  }

  this->AppendSelections = newAppendSelections;

  this->updateLabels();
  Q_EMIT this->selectionChanged(this->AppendSelections);
}

//-----------------------------------------------------------------------------
void pqSelectionInputWidget::onActiveSelectionChanged()
{
  // The selection has changed, either a new selection was created
  // or an old one cleared.
  this->Ui->label->setText("Copied Selection (Active Selection Changed)");
}

//-----------------------------------------------------------------------------
void pqSelectionInputWidget::postAccept()
{
  // Select proper ProxyManager
  vtkSMProxy* appendSelections = this->selection();
  vtkSMSessionProxyManager* pxm =
    appendSelections ? appendSelections->GetSessionProxyManager() : nullptr;

  if (!appendSelections)
  {
    return;
  }

  // Unregister any de-referenced proxy sources.
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSession(appendSelections->GetSession());
  for (iter->Begin("selection_sources"); !iter->IsAtEnd();)
  {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy->GetNumberOfConsumers() == 0)
    {
      const std::string key = iter->GetKey();
      iter->Next();
      pxm->UnRegisterProxy("selection_sources", key.c_str(), proxy);
    }
    else
    {
      iter->Next();
    }
  }
}

//-----------------------------------------------------------------------------
void pqSelectionInputWidget::preAccept()
{
  // Select proper ProxyManager
  vtkSMProxy* appendSelections = this->selection();
  vtkSMSessionProxyManager* pxm =
    appendSelections ? appendSelections->GetSessionProxyManager() : nullptr;

  if (appendSelections && pxm)
  {
    if (!pxm->GetProxyName("selection_sources", appendSelections))
    {
      // Register append selections proxy
      const std::string appendSelectionsKey =
        std::string("selection_filter.") + appendSelections->GetGlobalIDAsString();
      pxm->RegisterProxy("selection_sources", appendSelectionsKey.c_str(), appendSelections);
      //  Also register the inputs' proxy of append Selections
      auto inputs = vtkSMInputProperty::SafeDownCast(appendSelections->GetProperty("Input"));
      for (unsigned int i = 0; i < inputs->GetNumberOfProxies(); ++i)
      {
        vtkSMProxy* selectionSource = inputs->GetProxy(i);
        const std::string selectionSourceKey =
          std::string("selection_sources.") + selectionSource->GetGlobalIDAsString();
        pxm->RegisterProxy("selection_sources", selectionSourceKey.c_str(), selectionSource);
      }
    }
  }
}
