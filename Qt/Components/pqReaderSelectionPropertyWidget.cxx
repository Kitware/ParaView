/*=========================================================================

   Program: ParaView
   Module:    pqReaderSelectionPropertyWidget.cxx

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
#include "pqReaderSelectionPropertyWidget.h"

#include "pqArraySelectionWidget.h"
#include "pqSMAdaptor.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"
#include <vtksys/SystemTools.hxx>

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QList>
#include <QVariant>
#include <QtDebug>

class pqReaderSelectionPropertyWidget::pqInternals
{
public:
  QPointer<pqArraySelectionWidget> ArraySelectionWidget;

  void setArraySelectionWidget(pqArraySelectionWidget* wdg) { this->ArraySelectionWidget = wdg; }

  QList<QVariantList> getReaders(vtkSMProxy* smproxy)
  {
    // make a map of all known readers, with enabled/disabled state from the settings proxy.
    auto excludedProp =
      vtkSMStringVectorProperty::SafeDownCast(smproxy->GetProperty("ReaderSelection"));
    vtkNew<vtkStringList> excludedNames;
    if (excludedProp)
    {
      excludedProp->GetElements(excludedNames);
    }
    QList<QVariantList> props;
    vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
    vtkSMSession* session = vtkSMProxyManager::GetProxyManager()->GetActiveSession();
    if (session && readerFactory)
    {
      std::vector<FileTypeDetailed> filtersDetailed =
        readerFactory->GetSupportedFileTypesDetailed(session);
      for (auto& filterDetailed : filtersDetailed)
      {
        QVariantList tuple;
        if (filterDetailed.Description != "Supported Types" &&
          filterDetailed.Description != "All Files")
        {
          // add extensions like pqLoadDataReaction::loadData() does
          std::string extensions =
            " (" + vtksys::SystemTools::Join(filterDetailed.FilenamePatterns, " ") + ")";
          std::string filterDescr = filterDetailed.Description + extensions;
          tuple.push_back(filterDescr.c_str());
          // any readers which are unknown or not specifically excluded are enabled.
          if (excludedNames->GetIndex(filterDetailed.Description.c_str()) != -1)
          {
            tuple.push_back("0");
          }
          else
          {
            tuple.push_back("1");
          }
          props.push_back(tuple);
        }
      }
    }
    else
    {
      vtkErrorWithObjectMacro(nullptr, "No session");
    }
    return props;
  }
};

//-----------------------------------------------------------------------------
pqReaderSelectionPropertyWidget::pqReaderSelectionPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqReaderSelectionPropertyWidget::pqInternals())
{
  auto selectorWidget = new pqArraySelectionWidget(this);
  selectorWidget->setObjectName("ReaderSelectionWidget");
  selectorWidget->setHeaderLabel(
    QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));
  selectorWidget->setMaximumRowCountBeforeScrolling(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(smproperty->GetHints()));

  auto& internals = (*this->Internals);
  internals.setArraySelectionWidget(selectorWidget);

  // add context menu and custom indicator for sorting and filtering.
  new pqTreeViewSelectionHelper(selectorWidget);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setMargin(0);
  hbox->setSpacing(4);

  const char* property_name = smproxy->GetPropertyName(smproperty);

  if (auto hints = smproperty->GetHints()
      ? smproperty->GetHints()->FindNestedElementByName("ArraySelectionWidget")
      : nullptr)
  {
    selectorWidget->setIconType(property_name, hints->GetAttribute("icon_type"));
  }

  QVariant currentSMValue = QVariant::fromValue(this->Internals->getReaders(this->proxy()));
  selectorWidget->setProperty("AllNameFilters", currentSMValue);

  QObject::connect(selectorWidget, &pqArraySelectionWidget::widgetModified, this,
    &pqReaderSelectionPropertyWidget::readerListChanged);
}

void pqReaderSelectionPropertyWidget::readerListChanged()
{
  QVariant value = this->Internals->ArraySelectionWidget->property("AllNameFilters");
  // create a list of disabled readers
  std::vector<std::string> disabledReaders;
  const QList<QList<QVariant>> status_values = value.value<QList<QList<QVariant>>>();
  for (const QList<QVariant>& tuple : status_values)
  {
    if (tuple.size() == 2)
    {
      bool enabled = tuple[1].toBool();
      if (!enabled)
      {
        // strip extensions, store just the description
        std::string displayedEntry = tuple[0].toString().toStdString();
        std::string entryDescription = displayedEntry.substr(0, displayedEntry.rfind(" ("));
        disabledReaders.push_back(entryDescription);
      }
    }
  }
  // update the actual settings proxy
  auto excludedProp = vtkSMStringVectorProperty::SafeDownCast(this->property());
  if (excludedProp)
  {
    excludedProp->SetElements(disabledReaders);
  }
}

//-----------------------------------------------------------------------------
pqReaderSelectionPropertyWidget::~pqReaderSelectionPropertyWidget() = default;
