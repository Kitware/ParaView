// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFourLinesPropertyWidget.h"
#include "ui_pqFourLinesPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

class pqFourLinesPropertyWidget::pqInternals
{
public:
  Ui::FourLinesPropertyWidget Ui;
  pqInternals() = default;
};

//-----------------------------------------------------------------------------
pqFourLinesPropertyWidget::pqFourLinesPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "FourLinesWidgetRepresentation", smproxy, smgroup, parentObject)
  , Internals(new pqFourLinesPropertyWidget::pqInternals())
{
  Ui::FourLinesPropertyWidget& ui = this->Internals->Ui;
  ui.setupUi(this);

  if (vtkSMProperty* p1 = smgroup->GetProperty("Point1WorldPositions"))
  {
    ui.labelPoint1->setText(
      QCoreApplication::translate("ServerManagerXML", p1->GetXMLLabel()) + '1');
    this->addPropertyLink(ui.point1X, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 0);
    this->addPropertyLink(ui.point1Y, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 1);
    this->addPropertyLink(ui.point1Z, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 2);

    ui.labelPoint3->setText(
      QCoreApplication::translate("ServerManagerXML", p1->GetXMLLabel()) + '2');
    this->addPropertyLink(ui.point3X, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 3);
    this->addPropertyLink(ui.point3Y, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 4);
    this->addPropertyLink(ui.point3Z, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 5);

    ui.labelPoint5->setText(
      QCoreApplication::translate("ServerManagerXML", p1->GetXMLLabel()) + '3');
    this->addPropertyLink(ui.point5X, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 6);
    this->addPropertyLink(ui.point5Y, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 7);
    this->addPropertyLink(ui.point5Z, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 8);

    ui.labelPoint7->setText(
      QCoreApplication::translate("ServerManagerXML", p1->GetXMLLabel()) + '4');
    this->addPropertyLink(ui.point7X, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 9);
    this->addPropertyLink(ui.point7Y, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 10);
    this->addPropertyLink(ui.point7Z, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 11);
  }
  else
  {
    qCritical("Missing required property for function 'Point1WorldPositions'.");
  }

  if (vtkSMProperty* p2 = smgroup->GetProperty("Point2WorldPositions"))
  {
    ui.labelPoint2->setText(
      QCoreApplication::translate("ServerManagerXML", p2->GetXMLLabel()) + '1');
    this->addPropertyLink(ui.point2X, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 0);
    this->addPropertyLink(ui.point2Y, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 1);
    this->addPropertyLink(ui.point2Z, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 2);

    ui.labelPoint4->setText(
      QCoreApplication::translate("ServerManagerXML", p2->GetXMLLabel()) + '2');
    this->addPropertyLink(ui.point4X, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 3);
    this->addPropertyLink(ui.point4Y, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 4);
    this->addPropertyLink(ui.point4Z, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 5);

    ui.labelPoint6->setText(
      QCoreApplication::translate("ServerManagerXML", p2->GetXMLLabel()) + '3');
    this->addPropertyLink(ui.point6X, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 6);
    this->addPropertyLink(ui.point6Y, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 7);
    this->addPropertyLink(ui.point6Z, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 8);

    ui.labelPoint8->setText(
      QCoreApplication::translate("ServerManagerXML", p2->GetXMLLabel()) + '4');
    this->addPropertyLink(ui.point8X, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 9);
    this->addPropertyLink(ui.point8Y, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 10);
    this->addPropertyLink(ui.point8Z, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 11);
  }
  else
  {
    qCritical("Missing required property for function 'Point2WorldPositions'.");
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  this->connect(ui.switchL1, &QPushButton::clicked, this, [this]() { this->switchPoints(0); });
  this->connect(ui.switchL2, &QPushButton::clicked, this, [this]() { this->switchPoints(1); });
  this->connect(ui.switchL3, &QPushButton::clicked, this, [this]() { this->switchPoints(2); });
  this->connect(ui.switchL4, &QPushButton::clicked, this, [this]() { this->switchPoints(3); });

  pqCoreUtilities::connect(
    this->widgetProxy(), vtkCommand::PropertyModifiedEvent, this, SLOT(updateLengthLabels()));
  this->updateLengthLabels();
}

//-----------------------------------------------------------------------------
pqFourLinesPropertyWidget::~pqFourLinesPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqFourLinesPropertyWidget::updateLengthLabels()
{
  Ui::FourLinesPropertyWidget& ui = this->Internals->Ui;

  this->updateLengthLabel(ui.labelLength1, 0);
  this->updateLengthLabel(ui.labelLength2, 1);
  this->updateLengthLabel(ui.labelLength3, 2);
  this->updateLengthLabel(ui.labelLength4, 3);
}

//-----------------------------------------------------------------------------
void pqFourLinesPropertyWidget::updateLengthLabel(QLabel* lengthLabel, int index)
{
  vtkSMProxy* wproxy = this->widgetProxy();

  double p1List[12];
  vtkSMPropertyHelper(wproxy, "Point1WorldPositions").Get(p1List, 12);

  double p2List[12];
  vtkSMPropertyHelper(wproxy, "Point2WorldPositions").Get(p2List, 12);

  double* p1 = &p1List[3 * index];
  double* p2 = &p2List[3 * index];

  double distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  lengthLabel->setText(QString("<b>%1</b> <i>%2</i> ").arg(tr("Length: ")).arg(distance));
}

//-----------------------------------------------------------------------------
void pqFourLinesPropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
void pqFourLinesPropertyWidget::switchPoints(int index)
{

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();

  double p1List[12];
  vtkSMPropertyHelper(wdgProxy, "Point1WorldPositions").Get(p1List, 12);

  double p2List[12];
  vtkSMPropertyHelper(wdgProxy, "Point2WorldPositions").Get(p2List, 12);

  for (int i = 3 * index; i < 3 * index + 3; i++)
  {
    vtkSMPropertyHelper(wdgProxy, "Point1WorldPositions").Set(i, p2List[i]);
    vtkSMPropertyHelper(wdgProxy, "Point2WorldPositions").Set(i, p1List[i]);
  }

  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqFourLinesPropertyWidget::setLineColor(const QColor& color)
{
  vtkSMProxy* widget = this->widgetProxy();
  vtkSMPropertyHelper(widget, "LineColor").Set(0, color.redF());
  vtkSMPropertyHelper(widget, "LineColor").Set(1, color.greenF());
  vtkSMPropertyHelper(widget, "LineColor").Set(2, color.blueF());
  widget->UpdateVTKObjects();
}
