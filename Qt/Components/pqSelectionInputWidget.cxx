// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSelectionInputWidget.h"
#include "ui_pqSelectionInputWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
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
    this->Ui->label->setText(tr("No selection"));
    this->Ui->textBrowser->setText("");
    return;
  }

  this->Ui->label->setText(tr("Copied Selection"));

  // the selection source should be an appendSelections
  auto& appendSelections = this->AppendSelections;
  unsigned int numInputs = appendSelections && appendSelections->GetProperty("Input")
    ? vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements()
    : 0;
  QString text;
  QTextStream columnValues(&text, QIODevice::ReadWrite);
  if (numInputs > 0)
  {
    columnValues << tr("Number of Selections: ") << numInputs << QT_ENDL;
    columnValues << tr("Selection Expression: ")
                 << vtkSMPropertyHelper(appendSelections, "Expression").GetAsString() << QT_ENDL;
    columnValues << tr("Invert Selection: ")
                 << (vtkSMPropertyHelper(appendSelections, "InsideOut").GetAsInt() ? tr("Yes")
                                                                                   : tr("No"))
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
    columnValues << tr("Elements: ") << fieldTypeAsString << QT_ENDL;
    for (unsigned int i = 0; i < numInputs; ++i)
    {
      auto selectionSource = vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(i);
      auto selectionName = vtkSMPropertyHelper(appendSelections, "SelectionNames").GetAsString(i);
      const auto proxyXMLName = selectionSource->GetXMLName();
      columnValues << QT_ENDL << selectionName << ")" << QT_ENDL << tr("Type: ");
      if (strcmp(proxyXMLName, "FrustumSelectionSource") == 0)
      {
        columnValues << tr("Frustum Selection") << QT_ENDL << QT_ENDL << tr("Values:") << QT_ENDL
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
        columnValues << tr("Values Selection") << QT_ENDL << QT_ENDL << tr("Array Name: ")
                     << vtkSMPropertyHelper(selectionSource, "ArrayName").GetAsString() << QT_ENDL
                     << QT_ENDL << tr("Values") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("Pedigree ID Selection") << QT_ENDL << QT_ENDL;
        vtkSMProperty* dsivp = selectionSource->GetProperty("StringIDs");
        QList<QVariant> stringIdValues = pqSMAdaptor::getMultipleElementProperty(dsivp);
        if (!stringIdValues.empty())
        {
          columnValues << tr("Pedigree Domain") << "\t" << tr("String ID") << QT_ENDL << QT_ENDL;
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
          columnValues << tr("Pedigree Domain") << "\t" << tr("ID") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("Global ID Selection") << QT_ENDL << QT_ENDL << tr("Global ID")
                     << QT_ENDL << QT_ENDL;
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
        columnValues << tr("ID Selection") << QT_ENDL << QT_ENDL << tr("Process ID") << "\t"
                     << tr("Index") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("ID Selection") << QT_ENDL << QT_ENDL << tr("Composite ID") << "\t"
                     << tr("Process ID") << "\t" << tr("Index") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("ID Selection") << QT_ENDL << QT_ENDL;
        columnValues << tr("Level") << "\t" << tr("Dataset") << "\t" << tr("Index") << QT_ENDL
                     << QT_ENDL;
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
        columnValues << tr("Location-based Selection") << QT_ENDL << QT_ENDL
                     << tr("Probe Locations") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("Block Selection") << QT_ENDL << QT_ENDL << tr("Blocks") << QT_ENDL
                     << QT_ENDL;
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
          << tr("Block Selectors Selection") << QT_ENDL << QT_ENDL << tr("Assembly Name: ")
          << vtkSMPropertyHelper(selectionSource, "BlockSelectorsAssemblyName").GetAsString()
          << QT_ENDL << QT_ENDL << tr("Block Selectors") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("Threshold Selection") << QT_ENDL << QT_ENDL << tr("Array Name: ")
                     << vtkSMPropertyHelper(selectionSource, "ArrayName").GetAsString() << QT_ENDL
                     << QT_ENDL << tr("Thresholds") << QT_ENDL << QT_ENDL;
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
        columnValues << tr("Query Selection") << QT_ENDL << QT_ENDL << tr("Query: ")
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
    columnValues << tr("Number of Selections: 0") << QT_ENDL;
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

  vtkSmartPointer<vtkSMProxy> newSource;
  vtkSMSessionProxyManager* pxm = nullptr;

  // Ensure that the selection set by the widget will be in the same server as the proxy which
  // provide this widget
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (pqLiveInsituManager::isInsituServer(server))
  {
    pxm = server->session()->GetSessionProxyManager();
  }
  else
  {
    pxm = activeSelection->GetSessionProxyManager();
  }

  if (!pxm)
  {
    qWarning() << "Can't find suitable Session Proxy Manager to send the active selection";
    return;
  }

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
  this->preAccept();
  Q_EMIT this->selectionChanged(this->AppendSelections);
}

//-----------------------------------------------------------------------------
void pqSelectionInputWidget::onActiveSelectionChanged()
{
  // The selection has changed, either a new selection was created
  // or an old one cleared.
  this->Ui->label->setText(tr("Copied Selection (Active Selection Changed)"));
  this->preAccept();
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
