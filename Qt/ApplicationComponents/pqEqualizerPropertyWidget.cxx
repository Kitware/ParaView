// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEqualizerPropertyWidget.h"

#include "vtkEqualizerContextItem.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqLineEdit.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqProxyWidget.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

//-----------------------------------------------------------------------------
struct pqEqualizerPropertyWidget::pqInternals
{
  static constexpr int DefaultSampleFreq = 1000;

  pqLineEdit* pointsLE = nullptr;
  QCheckBox* visibilityCB = nullptr;
  QPushButton* savePB = nullptr;
  QPushButton* loadPB = nullptr;
  QPushButton* resetPB = nullptr;
  vtkWeakPointer<vtkEqualizerContextItem> EqualizerItem;
};

//-----------------------------------------------------------------------------
pqEqualizerPropertyWidget::pqEqualizerPropertyWidget(
  vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent)
  : Superclass("representations", "EqualizerWidgetRepresentation", proxy, smgroup, parent)
  , Internals(new pqInternals)
{
  // Expected and optional properties
  vtkSMStringVectorProperty* pointsProp =
    vtkSMStringVectorProperty::SafeDownCast(smgroup->GetProperty("EqualizerPoints"));
  vtkSMIntVectorProperty* samplingFreqProp =
    vtkSMIntVectorProperty::SafeDownCast(smgroup->GetProperty("SamplingFrequency"));

  // Setup GUI
  QGridLayout* layout = new QGridLayout(this);
  layout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  layout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  layout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  QCheckBox* visibility = new QCheckBox(tr("Visibility"), this);
  layout->addWidget(visibility);

  this->Internals->savePB = new QPushButton(tr("Save"), this);
  this->Internals->loadPB = new QPushButton(tr("Load"), this);
  this->Internals->resetPB = new QPushButton(tr("Reset"), this);
  layout->addWidget(this->Internals->savePB, 1, 0);
  layout->addWidget(this->Internals->loadPB, 1, 1);
  layout->addWidget(this->Internals->resetPB, 1, 2);

  this->Internals->pointsLE = new pqLineEdit(this);
  this->Internals->pointsLE->setEnabled(false);
  layout->addWidget(this->Internals->pointsLE, 2, 0, 1, 3);

  if (samplingFreqProp)
  {
    auto* samplingFreqLabel = new QLabel(
      QCoreApplication::translate("ServerManagerXML", samplingFreqProp->GetXMLLabel()), this);
    auto* samplingFreqWidget =
      pqProxyWidget::createWidgetForProperty(samplingFreqProp, proxy, this);
    QObject::connect(samplingFreqWidget, &pqPropertyWidget::changeAvailable, this,
      &pqPropertyWidget::changeAvailable);
    QObject::connect(samplingFreqWidget, &pqPropertyWidget::changeFinished, this,
      &pqPropertyWidget::changeFinished);
    this->addPropertyLink(samplingFreqWidget, "text2", SIGNAL(changeFinished()), samplingFreqProp);

    layout->addWidget(samplingFreqLabel, 3, 0, 1, 0);
    layout->addWidget(samplingFreqWidget, 3, 1, 1, 2);
  }
  this->setWidgetVisible(visibility->isChecked());

  // Setup GUI connections
  QObject::connect(this->Internals->savePB, &QPushButton::clicked, this,
    &pqEqualizerPropertyWidget::saveEqualizer);
  QObject::connect(this->Internals->loadPB, &QPushButton::clicked, this,
    &pqEqualizerPropertyWidget::loadEqualizer);
  QObject::connect(this->Internals->resetPB, &QPushButton::clicked, this,
    &pqEqualizerPropertyWidget::resetEqualizer);
  QObject::connect(this, &pqEqualizerPropertyWidget::startInteraction, this,
    &pqEqualizerPropertyWidget::updatePosition);
  QObject::connect(this, &pqEqualizerPropertyWidget::interaction, this,
    &pqEqualizerPropertyWidget::updatePosition);
  QObject::connect(this, &pqEqualizerPropertyWidget::endInteraction, this,
    &pqEqualizerPropertyWidget::updatePosition);
  QObject::connect(
    this, &pqEqualizerPropertyWidget::widgetVisibilityToggled, visibility, &QCheckBox::setChecked);
  QObject::connect(
    visibility, &QCheckBox::toggled, this, &pqEqualizerPropertyWidget::setWidgetVisible);

  if (pointsProp)
  {
    // Initialize link with widget properties
    auto* wdgProxy = this->widgetProxy();
    this->addPropertyLink(
      this->Internals->pointsLE, "text2", SIGNAL(textChangedAndEditingFinished()), pointsProp, 0);
    this->WidgetLinks.addPropertyLink(this->Internals->pointsLE, "text2",
      SIGNAL(textChangedAndEditingFinished()), wdgProxy, wdgProxy->GetProperty("EqualizerPoints"));

    // Initialize widget position
    std::string points = pointsProp->GetElement(0);
    if (points.empty())
    {
      auto frequency =
        samplingFreqProp ? samplingFreqProp->GetElement(0) : pqInternals::DefaultSampleFreq;
      QString init_points(QString("0,1; %1,1;").arg(frequency / 2));
      this->Internals->pointsLE->setText(init_points);
      points = init_points.toStdString();
    }
    else
    {
      this->Internals->pointsLE->setText(QString::fromStdString(points));
    }

    vtkSMProxy* contextProxy = wdgProxy->GetContextItemProxy();
    this->Internals->EqualizerItem =
      vtkEqualizerContextItem::SafeDownCast(contextProxy->GetClientSideObject());
    this->Internals->EqualizerItem->SetPoints(points);
  }
}

//-----------------------------------------------------------------------------
pqEqualizerPropertyWidget::~pqEqualizerPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::placeWidget()
{
  this->widgetProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::saveEqualizer()
{
  pqFileDialog dialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Save Equalizer:"), QString(), "CSV (*.csv)", false);
  dialog.setFileMode(pqFileDialog::AnyFile);
  if (QFileDialog::Accepted == dialog.exec())
  {
    QStringList files = dialog.getSelectedFiles();
    const QString& fileName = files.first();
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate))
    {
      QTextStream stream(&file);
      QString points = QString::fromStdString(this->Internals->EqualizerItem->GetPoints());
      QStringList pointsList = points.split(';');
      for (const QString& point : pointsList)
      {
        stream << point << "\n";
      }
    }
    file.close();
  }
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::loadEqualizer()
{
  pqFileDialog dialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Load Equalizer:"), QString(), "CSV (*.csv)", false);
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (QFileDialog::Accepted == dialog.exec())
  {
    QStringList files = dialog.getSelectedFiles();
    const QString& fileName = files.first();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
      vtkGenericWarningMacro("Failed to load equalizer file:\n"
        << file.errorString().toStdString());
      return;
    }

    QString points;
    while (!file.atEnd())
    {
      QByteArray line = file.readLine();
      points.append(line);
      points.append(';');
    }

    file.close();
    this->Internals->pointsLE->setText(points);
    this->Internals->EqualizerItem->SetPoints(points.toStdString());
    this->proxy()->UpdateVTKObjects();
    this->proxy()->Modified();
    Q_EMIT changeAvailable();
  }
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::resetEqualizer()
{
  if (!this->Internals->pointsLE || !this->Internals->EqualizerItem)
  {
    return;
  }

  vtkSMIntVectorProperty* samplingFreqProp =
    vtkSMIntVectorProperty::SafeDownCast(this->propertyGroup()->GetProperty("SamplingFrequency"));

  int frequency =
    samplingFreqProp ? samplingFreqProp->GetElement(0) : pqInternals::DefaultSampleFreq;

  // We divide the sampling frequency by 2 so it fits with the actual range after the FFT
  QString init_points(QString("0,1; %1,1;").arg(frequency / 2));
  this->Internals->pointsLE->setText(init_points);
  auto points = init_points.toStdString();
  this->Internals->EqualizerItem->SetPoints(points);
  Q_EMIT changeAvailable();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::updatePosition()
{
  if (!this->Internals->EqualizerItem)
  {
    return;
  }

  auto points = this->Internals->EqualizerItem->GetPoints();
  this->Internals->pointsLE->setText(QString::fromStdString(points));
}
