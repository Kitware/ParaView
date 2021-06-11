/*=========================================================================

  Program:   ParaView
  Module:    pqEqualizerPropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqEqualizerPropertyWidget.h"

#include "vtkEqualizerContextItem.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNew2DWidgetRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqFileDialog.h"
#include "pqIntRangeWidget.h"
#include "pqLineEdit.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QPushButton>

//-----------------------------------------------------------------------------
class pqEqualizerPropertyWidget::pqInternals
{
public:
  pqLineEdit* pointsLE;
  QCheckBox* visibilityCB;
  QPushButton* savePB;
  QPushButton* loadPB;
  QPushButton* resetPB;
  vtkWeakPointer<vtkEqualizerContextItem> EqualizerItem;
};

//-----------------------------------------------------------------------------
pqEqualizerPropertyWidget::pqEqualizerPropertyWidget(
  vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent)
  : Superclass("representations", "EqualizerWidgetRepresentation", proxy, smgroup, parent)
  , Internals(new pqInternals())
{
  Init(proxy, smgroup);
}

//-----------------------------------------------------------------------------
pqEqualizerPropertyWidget::~pqEqualizerPropertyWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::placeWidget()
{
  vtkSMNew2DWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::onStartInteraction()
{
  UpdatePosition();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::onInteraction()
{
  UpdatePosition();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::onEndInteraction()
{
  UpdatePosition();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::saveEqualizer()
{
  pqFileDialog dialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Save Equalizer:"), QString(), "CSV (*.csv)");
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
        stream << point << "\n";
    }
    file.close();
  }
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::loadEqualizer()
{
  pqFileDialog dialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Load Equalizer:"), QString(), "CSV (*.csv)");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (QFileDialog::Accepted == dialog.exec())
  {
    QStringList files = dialog.getSelectedFiles();
    const QString& fileName = files.first();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
      qDebug() << file.errorString();
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
  if (!this->Internals->pointsLE)
  {
    return;
  }

  if (!this->Internals->EqualizerItem)
  {
    return;
  }

  vtkSMIntVectorProperty* sapmlingFreqProp =
    vtkSMIntVectorProperty::SafeDownCast(this->proxy()->GetProperty("SamplingFrequency"));

  auto frequency = sapmlingFreqProp ? sapmlingFreqProp->GetElement(0) : 1000;

  QString init_points(QString("0,1; %1,1;").arg(frequency / 2));
  this->Internals->pointsLE->setText(init_points);
  auto points = init_points.toStdString();
  this->Internals->EqualizerItem->SetPoints(points);
  Q_EMIT changeAvailable();
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::Init(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup)
{
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
    vtkSMUncheckedPropertyHelper(this->proxy(), "Input").GetAsProxy(0));
  if (!input)
  {
    return;
  }

  vtkSMNew2DWidgetRepresentationProxy* wdgProxy = this->widgetProxy();

  QGridLayout* layout = new QGridLayout(this);
  layout->setMargin(pqPropertiesPanel::suggestedMargin());
  layout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  layout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  auto rowId = 0;

  QCheckBox* visibility = new QCheckBox(tr("Visibility"), this);
  layout->addWidget(visibility);
  ++rowId;

  this->connect(visibility, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  visibility->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(visibility->isChecked());

  this->Internals->savePB = new QPushButton(tr("Save"), this);
  this->Internals->loadPB = new QPushButton(tr("Load"), this);
  this->Internals->resetPB = new QPushButton(tr("Reset"), this);
  layout->addWidget(this->Internals->savePB, rowId, 0);
  layout->addWidget(this->Internals->loadPB, rowId, 1);
  layout->addWidget(this->Internals->resetPB, rowId, 2);
  connect(this->Internals->savePB, &QPushButton::clicked, this,
    &pqEqualizerPropertyWidget::saveEqualizer);
  connect(this->Internals->loadPB, &QPushButton::clicked, this,
    &pqEqualizerPropertyWidget::loadEqualizer);
  connect(this->Internals->resetPB, &QPushButton::clicked, this,
    &pqEqualizerPropertyWidget::resetEqualizer);

  this->Internals->pointsLE = new pqLineEdit(this);
  this->Internals->pointsLE->setEnabled(false);
  layout->addWidget(this->Internals->pointsLE, ++rowId, 0, 1, 3);

  vtkPVDataSetAttributesInformation* fdi = input->GetDataInformation(0)->GetRowDataInformation();
  if (!fdi)
  {
    return;
  }
  vtkPVArrayInformation* array_info = fdi->GetArrayInformation(0);
  if (!array_info)
  {
    return;
  }

  if (vtkSMProperty* p1 = smgroup->GetProperty("EqualizerPointsFunc"))
  {
    this->addPropertyLink(
      this->Internals->pointsLE, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 0);
    this->WidgetLinks.addPropertyLink(this->Internals->pointsLE, "text2",
      SIGNAL(textChangedAndEditingFinished()), wdgProxy,
      wdgProxy->GetProperty("EqualizerPointsFunc"));
  }

  connect(this, SIGNAL(startInteraction()), this, SLOT(onStartInteraction()));
  connect(this, SIGNAL(interaction()), this, SLOT(onInteraction()));
  connect(this, SIGNAL(endInteraction()), this, SLOT(onEndInteraction()));

  vtkSMStringVectorProperty* pointsProp =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("EqualizerPoints"));
  std::string points = pointsProp->GetElement(0);
  if (points.empty())
  {
    vtkSMIntVectorProperty* sapmlingFreqProp =
      vtkSMIntVectorProperty::SafeDownCast(this->proxy()->GetProperty("SamplingFrequency"));

    auto frequency = sapmlingFreqProp ? sapmlingFreqProp->GetElement(0) : 1000;
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

  smgroup->GetProperty("EqualizerPointsFunc")->Modified();

  connect(visibility, &QCheckBox::toggled, [this](bool visible) {
    if (visible)
      this->Internals->EqualizerItem->SetPoints(this->Internals->pointsLE->text().toStdString());
  });
}

//-----------------------------------------------------------------------------
void pqEqualizerPropertyWidget::UpdatePosition()
{
  if (!this->Internals->EqualizerItem)
    return;
  auto points = this->Internals->EqualizerItem->GetPoints();
  this->Internals->pointsLE->setText(QString::fromStdString(points));
}
