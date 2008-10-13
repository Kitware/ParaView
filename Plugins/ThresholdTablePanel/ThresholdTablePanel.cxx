#include "ThresholdTablePanel.h"

#include "pqDoubleRangeWidget.h"
#include <pqProxy.h>
#include <pqPropertyHelper.h>
#include "pqSMAdaptor.h"

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>

#include <iostream>
#include <set>

ThresholdTablePanel::ThresholdTablePanel(pqProxy* proxy, QWidget* p) :
  pqNamedObjectPanel(proxy, p)
  //pqLoadedFormObjectPanel(":/ThresholdTablePanel/ThresholdTablePanel.ui", proxy, p)
{
  this->Widgets.setupUi(this);

  //this->Lower = this->findChild<pqDoubleRangeWidget*>("ThresholdBetween_0");
  //this->Upper = this->findChild<pqDoubleRangeWidget*>("ThresholdBetween_1");
  this->Lower = this->Widgets.ThresholdBetween_0;
  this->Upper = this->Widgets.ThresholdBetween_1;

  QObject::connect(this->Lower, SIGNAL(valueEdited(double)),
                   this, SLOT(lowerChanged(double)));
  QObject::connect(this->Upper, SIGNAL(valueEdited(double)),
                   this, SLOT(upperChanged(double)));

  //QObject::connect(this->findChild<QComboBox*>("Column"),
  QObject::connect(this->Widgets.Column,
    SIGNAL(activated(int)), this, SLOT(variableChanged()),
    Qt::QueuedConnection);

  this->linkServerManagerProperties();
}

void ThresholdTablePanel::accept()
{
  Superclass::accept();    
}

void ThresholdTablePanel::reset()
{
  Superclass::reset();
}

void ThresholdTablePanel::lowerChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->Upper->value() < val)
    {
    this->Upper->setValue(val);
    }
}

void ThresholdTablePanel::upperChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->Lower->value() > val)
    {
    this->Lower->setValue(val);
    }
}

void ThresholdTablePanel::variableChanged()
{
  // when the user changes the variable, adjust the ranges on the ThresholdBetween
  vtkSMProperty* prop = this->proxy()->GetProperty("ThresholdBetween");
  QList<QVariant> range = pqSMAdaptor::getElementPropertyDomain(prop);
  if(range.size() == 2 && range[0].isValid() && range[1].isValid())
    {
    this->Lower->setValue(range[0].toDouble());
    this->Upper->setValue(range[1].toDouble());
    }
}

