// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDoubleVectorPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqDiscreteDoubleWidget.h"
#include "pqDoubleLineEdit.h"
#include "pqDoubleRangeWidget.h"
#include "pqHighlightableToolButton.h"
#include "pqLabel.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqScalarValueListPropertyWidget.h"
#include "pqScaleByButton.h"
#include "pqSignalAdaptorSelectionTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqWidgetRangeDomain.h"

#include "vtkBoundingBox.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkPVDataInformation.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDiscreteDoubleDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMTimeStepsDomain.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSmartPointer.h"

#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidgetItem>

//-----------------------------------------------------------------------------
pqDoubleVectorPropertyWidget::pqDoubleVectorPropertyWidget(
  vtkSMProperty* smProperty, vtkSMProxy* smProxy, QWidget* parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  this->setProperty(smProperty);
  this->setChangeAvailableAsChangeFinished(false);

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(smProperty);
  if (!dvp)
  {
    return;
  }

  // find the domain
  vtkSmartPointer<vtkSMDoubleRangeDomain> defaultDomain;

  vtkSMDomain* domain = nullptr;
  vtkSMDomainIterator* domainIter = dvp->NewDomainIterator();
  for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
  {
    domain = domainIter->GetDomain();
  }
  domainIter->Delete();

  if (!domain)
  {
    defaultDomain = vtkSmartPointer<vtkSMDoubleRangeDomain>::New();
    domain = defaultDomain;
  }

  QHBoxLayout* layoutLocal = new QHBoxLayout;
  layoutLocal->setContentsMargins(0, 0, 0, 0);
  layoutLocal->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  this->setLayout(layoutLocal);

  // Fill Layout
  vtkPVXMLElement* hints = dvp->GetHints();
  bool isSelectable = false;
  bool showLabel = false;
  vtkPVXMLElement* showComponentLabels = nullptr;
  if (hints != nullptr)
  {
    isSelectable = (hints->FindNestedElementByName("IsSelectable") != nullptr);
    showLabel = (hints->FindNestedElementByName("ShowLabel") != nullptr);
    showComponentLabels = hints->FindNestedElementByName("ShowComponentLabels");
  }
  const int elementCount = dvp->GetNumberOfElements();
  const auto componentLabels = showComponentLabels
    ? pqPropertyWidget::parseComponentLabels(showComponentLabels, elementCount)
    : std::vector<std::string>{};

  vtkSMDoubleRangeDomain* range = vtkSMDoubleRangeDomain::SafeDownCast(domain);
  vtkSMTimeStepsDomain* tsDomain = vtkSMTimeStepsDomain::SafeDownCast(domain);
  if (this->property()->GetRepeatable())
  {
    if (isSelectable)
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
        "use a multi-select value list (in a `pqTreeWidget`)");
      pqTreeWidget* treeWidget = new pqTreeWidget(this);
      treeWidget->setObjectName("TreeWidget");
      treeWidget->setColumnCount(1);
      treeWidget->setRootIsDecorated(false);
      treeWidget->setMaximumRowCountBeforeScrolling(smProperty);

      QTreeWidgetItem* header = new QTreeWidgetItem();
      header->setData(0, Qt::DisplayRole,
        QCoreApplication::translate("ServerManagerXML", smProperty->GetXMLLabel()));
      treeWidget->setHeaderItem(header);

      // helper makes it easier to select multiple entries.
      pqTreeWidgetSelectionHelper* helper = new pqTreeWidgetSelectionHelper(treeWidget);
      helper->setObjectName("TreeWidgetSelectionHelper");

      // adaptor makes it possible to link with the smproperty.
      pqSignalAdaptorSelectionTreeWidget* adaptor =
        new pqSignalAdaptorSelectionTreeWidget(treeWidget, smProperty);
      adaptor->setObjectName("SelectionTreeWidgetAdaptor");
      this->addPropertyLink(adaptor, "values", SIGNAL(valuesChanged()), smProperty);

      layoutLocal->addWidget(treeWidget);
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
        "use `pqScalarValueListPropertyWidget` since property is repeatable");

      pqScalarValueListPropertyWidget* widget =
        new pqScalarValueListPropertyWidget(smProperty, this->proxy(), this);
      widget->setObjectName("ScalarValueList");
      this->addPropertyLink(widget, "scalars", SIGNAL(scalarsChanged()), smProperty);
      if (range)
      {
        widget->setRangeDomain(range);
      }
      else if (tsDomain)
      {
        widget->setRangeDomain(tsDomain);
        auto tsValues = tsDomain->GetValues();

        // Initialize the scalar value list using the timesteps of the domain
        QList<QVariant> tsList;
        tsList.reserve(static_cast<int>(tsValues.size()));
        std::copy(tsValues.begin(), tsValues.end(), std::back_inserter(tsList));
        widget->setScalars(tsList);
      }
      widget->setShowLabels(showComponentLabels);
      if (showComponentLabels)
      {
        widget->setLabels(componentLabels);
      }

      layoutLocal->addWidget(widget);
    }
    this->setChangeAvailableAsChangeFinished(true);
    this->setShowLabel(showLabel);
  }
  else if (range)
  {
    if (dvp->GetNumberOfElements() == 1 &&
      ((range->GetMinimumExists(0) && range->GetMaximumExists(0)) ||
        (dvp->FindDomain<vtkSMArrayRangeDomain>() != nullptr ||
          dvp->FindDomain<vtkSMBoundsDomain>() != nullptr)))
    {
      // bounded ranges are represented with a slider and a spin box
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
        "use `pqDoubleRangeWidget` since property has range with min and max");

      pqDoubleRangeWidget* widget = new pqDoubleRangeWidget(this);
      widget->setObjectName("DoubleRangeWidget");
      widget->setUseGlobalPrecisionAndNotation(true);
      widget->setMinimum(range->GetMinimum(0));
      widget->setMaximum(range->GetMaximum(0));
      if (range->GetResolutionExists())
      {
        widget->setResolution(range->GetResolution());
      }

      // ensures that the widget's range is updated whenever the domain changes.
      new pqWidgetRangeDomain(widget, "minimum", "maximum", dvp, 0);

      this->addPropertyLink(widget, "value", SIGNAL(valueChanged(double)), smProperty);
      this->connect(widget, SIGNAL(valueEdited(double)), this, SIGNAL(changeFinished()));

      layoutLocal->addWidget(widget, 1);
    }
    else
    {
      // unbounded ranges are represented with a line edit
      if (elementCount == 6)
      {
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use 6 `pqDoubleLineEdit` in a 3X2 grid.");
        QGridLayout* gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(0);
        gridLayout->setVerticalSpacing(2);

        for (int i = 0; i < 3; i++)
        {
          pqDoubleLineEdit* lineEdit = new pqDoubleLineEdit(this);
          lineEdit->setUseGlobalPrecisionAndNotation(true);
          lineEdit->setObjectName(QString("DoubleLineEdit%1").arg(2 * i));
          if (showComponentLabels)
          {
            pqLabel* label = new pqLabel(componentLabels[2 * i].c_str(), this);
            label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
            gridLayout->addWidget(label, (i * 2), 0);
            gridLayout->addWidget(lineEdit, (i * 2) + 1, 0);
          }
          else
          {
            gridLayout->addWidget(lineEdit, i, 0);
          }
          this->addPropertyLink(lineEdit, "text2", SIGNAL(textChanged(const QString&)), dvp, 2 * i);
          this->connect(
            lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));

          lineEdit = new pqDoubleLineEdit(this);
          lineEdit->setObjectName(QString("DoubleLineEdit%1").arg(2 * i + 1));
          lineEdit->setUseGlobalPrecisionAndNotation(true);
          if (showComponentLabels)
          {
            pqLabel* label = new pqLabel(componentLabels[2 * i + 1].c_str(), this);
            label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
            gridLayout->addWidget(label, (i * 2), 1);
            gridLayout->addWidget(lineEdit, (i * 2) + 1, 1);
          }
          else
          {
            gridLayout->addWidget(lineEdit, i, 1);
          }
          this->addPropertyLink(
            lineEdit, "text2", SIGNAL(textChanged(const QString&)), dvp, 2 * i + 1);

          this->connect(
            lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
        }

        layoutLocal->addLayout(gridLayout);
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use %d `pqDoubleLineEdit` instance(s).",
          dvp->GetNumberOfElements());
        for (unsigned int i = 0; i < dvp->GetNumberOfElements(); i++)
        {
          if (showComponentLabels)
          {
            pqLabel* label = new pqLabel(componentLabels[i].c_str(), this);
            label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
            layoutLocal->addWidget(label);
          }
          pqDoubleLineEdit* lineEdit = new pqDoubleLineEdit(this);
          lineEdit->setObjectName(QString("DoubleLineEdit%1").arg(i));
          lineEdit->setUseGlobalPrecisionAndNotation(true);
          layoutLocal->addWidget(lineEdit);
          this->addPropertyLink(lineEdit, "text2", SIGNAL(textChanged(const QString&)), dvp, i);

          this->connect(
            lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
        }
      }
    }
  }
  else if (vtkSMDiscreteDoubleDomain* discrete = vtkSMDiscreteDoubleDomain::SafeDownCast(domain))
  {
    if (discrete->GetValuesExists())
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
        "use discrete slider widget `pqDiscreteDoubleWidget`");

      pqDiscreteDoubleWidget* widget = new pqDiscreteDoubleWidget(this);
      widget->setObjectName("DiscreteDoubleWidget");
      widget->setUseGlobalPrecisionAndNotation(true);
      widget->setValues(discrete->GetValues());

      this->addPropertyLink(widget, "value", SIGNAL(valueChanged(double)), smProperty);
      this->connect(widget, SIGNAL(valueEdited(double)), this, SIGNAL(changeFinished()));

      layoutLocal->addWidget(widget);
    }
    else
    {
      qCritical("vtkSMDiscreteDoubleDomain does not contain any value.");
    }
  }

  if (dvp->FindDomain<vtkSMArrayRangeDomain>() != nullptr ||
    dvp->FindDomain<vtkSMBoundsDomain>() != nullptr)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "add `Scale By` button");
    pqScaleByButton* scaleButton = new pqScaleByButton(this);
    scaleButton->setObjectName("ScaleBy");
    this->connect(scaleButton, SIGNAL(scale(double)), SLOT(scale(double)));
    layoutLocal->addWidget(scaleButton, 0, Qt::AlignBottom);

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "add `Reset-to-domain` button");

    // if this has an vtkSMArrayRangeDomain, add a "reset" button.
    pqHighlightableToolButton* resetButton = new pqHighlightableToolButton(this);
    resetButton->setObjectName("Reset");
    QAction* resetActn = new QAction(resetButton);
    resetActn->setToolTip(tr("Reset using current data values"));
    resetActn->setIcon(QIcon(":/pqWidgets/Icons/pqReset.svg"));
    resetButton->addAction(resetActn);
    resetButton->setDefaultAction(resetActn);

    pqCoreUtilities::connect(
      dvp, vtkCommand::DomainModifiedEvent, this, SIGNAL(highlightResetButton()));
    pqCoreUtilities::connect(
      dvp, vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(highlightResetButton()));

    this->connect(resetButton, SIGNAL(clicked()), SLOT(resetButtonClicked()));
    resetButton->connect(this, SIGNAL(highlightResetButton()), SLOT(highlight()));
    resetButton->connect(this, SIGNAL(clearHighlight()), SLOT(clear()));

    layoutLocal->addWidget(resetButton, 0, Qt::AlignBottom);
  }

  auto dvpHints = hints ? hints->FindNestedElementByName("DoubleVectorPropertyWidget") : nullptr;
  for (unsigned int cc = 0, max = (dvpHints ? dvpHints->GetNumberOfNestedElements() : 0); cc < max;
       ++cc)
  {
    auto elem = dvpHints->GetNestedElement(cc);
    if (elem && elem->GetName() && strcmp(elem->GetName(), "Button") == 0)
    {
      auto type = elem->GetAttributeOrEmpty("type");
      if (strcmp(type, "use_active_source_bounds") == 0)
      {
        auto tb = new QToolButton(this);
        tb->setObjectName(type);
        auto actn = new QAction(tb);
        tb->addAction(actn);
        tb->setDefaultAction(actn);
        tb->setToolTip(tr("Reset to active data bounds"));
        tb->setIcon(QIcon(":/pqWidgets/Icons/pqZoomToData.svg"));
        QObject::connect(
          tb, &QToolButton::clicked, this, &pqDoubleVectorPropertyWidget::resetToActiveDataBounds);
        layoutLocal->addWidget(tb, 0, Qt::AlignBottom);
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
          "unknown button requested type='%s', ignoring.", type);
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqDoubleVectorPropertyWidget::~pqDoubleVectorPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::resetButtonClicked()
{
  if (vtkSMProperty* smproperty = this->property())
  {
    smproperty->ResetToDomainDefaults(/*use_unchecked_values*/ true);
    Q_EMIT this->changeAvailable();
    Q_EMIT this->changeFinished();
  }
  Q_EMIT this->clearHighlight();
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::apply()
{
  this->Superclass::apply();
  Q_EMIT this->clearHighlight();
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::reset()
{
  this->Superclass::reset();
  Q_EMIT this->clearHighlight();
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::scaleHalf()
{
  this->scale(0.5);
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::scaleTwice()
{
  this->scale(2.0);
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::scale(double factor)
{
  if (vtkSMProperty* smproperty = this->property())
  {
    vtkSMUncheckedPropertyHelper helper(smproperty);
    for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; cc++)
    {
      helper.Set(cc, helper.GetAsDouble(cc) * factor);
    }
    Q_EMIT this->changeAvailable();
    Q_EMIT this->changeFinished();
  }
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::resetToActiveDataBounds()
{
  vtkBoundingBox bbox;
  if (auto port = pqActiveObjects::instance().activePort())
  {
    if (auto dinfo = port->getDataInformation())
    {
      bbox.AddBounds(dinfo->GetBounds());
    }
  }

  if (bbox.IsValid())
  {
    double bds[6];
    bbox.GetBounds(bds);
    this->resetToBounds(bds);
  }
}

//-----------------------------------------------------------------------------
void pqDoubleVectorPropertyWidget::resetToBounds(const double bds[6])
{
  if (vtkSMProperty* smproperty = this->property())
  {
    vtkSMUncheckedPropertyHelper helper(smproperty);
    if (helper.GetNumberOfElements() != 6)
    {
      qWarning("Property must have 6 elements. Ignoring reset request.");
      return;
    }

    for (unsigned int cc = 0; cc < 6; cc++)
    {
      helper.Set(cc, bds[cc]);
    }
    Q_EMIT this->changeAvailable();
    Q_EMIT this->changeFinished();
  }
}
