// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqDynamicPropertiesWidget.h"
#include "pqDoubleRangeWidget.h"
#include "pqIntRangeWidget.h"
#include "pqPropertiesPanel.h"
#include "vtkDynamicProperties.h"
#include "vtkLogger.h"

#include "vtkSMDynamicPropertiesDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include "vtkNew.h"
#include "json/json.h"

#include <QByteArray>
#include <QCheckBox>
#include <QDebug>
#include <QDynamicPropertyChangeEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QSlider>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include <QCoreApplication>
#include <algorithm>
#include <qcontainerfwd.h>
#include <qnamespace.h>

namespace
{

//------------------------------------------------------------------------------
// Used to name Qt properties on widgets so that they can be mapped back to a
// key (used with QObject::sender() from slots).
const char keyPropertyName[] = "DynamicPropertiesKey";

//------------------------------------------------------------------------------
class RowWidget
{
public:
  RowWidget(pqDynamicPropertiesWidget* parent, const QString& key, vtkDynamicProperties::Type type);
  virtual ~RowWidget();
  virtual void deleteLater();
  virtual QVariant value() = 0;
  virtual bool setValue(const QVariant& v) = 0;

  QHBoxLayout* layout;
  QLabel* label;
  vtkDynamicProperties::Type type;
};

//------------------------------------------------------------------------------
RowWidget::RowWidget(
  pqDynamicPropertiesWidget* parent, const QString& key, vtkDynamicProperties::Type t)
  : layout(new QHBoxLayout)
  , label(new QLabel(key, parent))
  , type(t)
{
  this->label->setProperty(keyPropertyName, key);
  this->layout->setContentsMargins(0, 0, 0, 0);
}

//------------------------------------------------------------------------------
RowWidget::~RowWidget()
{
  // It's possible for this to not have a parent set if the widget was not
  // added to an external layout, so ensure that it gets cleaned up.
  if (this->layout)
  {
    this->layout->deleteLater();
    this->layout = nullptr;
  }
}

//------------------------------------------------------------------------------
void RowWidget::deleteLater()
{
  this->label->deleteLater();
}

//------------------------------------------------------------------------------
class RowWidgetBool : public RowWidget
{
public:
  RowWidgetBool(pqDynamicPropertiesWidget* parent, const QString& name,
    vtkDynamicProperties::Type type, const QString& description, bool state);
  void deleteLater() override;
  QVariant value() override;
  bool setValue(const QVariant& value) override;

  void setCheckState(bool state);

private:
  QCheckBox* checkbox;
};

//------------------------------------------------------------------------------
RowWidgetBool::RowWidgetBool(pqDynamicPropertiesWidget* parent, const QString& name,
  vtkDynamicProperties::Type type, const QString& description, bool state)
  : RowWidget(parent, name, type)
  , checkbox(new QCheckBox(QString(), parent))
{
  this->setCheckState(state);
  this->checkbox->setProperty(keyPropertyName, name);
  this->checkbox->setToolTip(description);
  this->layout->addWidget(this->checkbox);

  this->label->setToolTip(description);

  parent->connect(
    this->checkbox, SIGNAL(checkStateChanged(Qt::CheckState)), SLOT(updateProperty()));
}

//------------------------------------------------------------------------------
QVariant RowWidgetBool::value()
{
  return QVariant(this->checkbox->checkState() == Qt::Checked);
}

//------------------------------------------------------------------------------
bool RowWidgetBool::setValue(const QVariant& value)
{
  this->checkbox->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
  return true;
}

//------------------------------------------------------------------------------
void RowWidgetBool::deleteLater()
{
  this->RowWidget::deleteLater();
  this->checkbox->deleteLater();
}

//------------------------------------------------------------------------------
void RowWidgetBool::setCheckState(bool state)
{
  bool oldBlock = this->checkbox->blockSignals(true);
  this->checkbox->setCheckState(state ? Qt::Checked : Qt::Unchecked);
  this->checkbox->blockSignals(oldBlock);
}

//------------------------------------------------------------------------------
class RowWidgetInt : public RowWidget
{
public:
  RowWidgetInt(pqDynamicPropertiesWidget* parent, const QString& key,
    vtkDynamicProperties::Type type, const QString& description, int minValue, int maxValue,
    int defaultValue);
  void deleteLater() override;
  QVariant value() override;
  bool setValue(const QVariant& value) override;
  void setValue(int);

private:
  pqIntRangeWidget* intRangeWidget;
};

//------------------------------------------------------------------------------
RowWidgetInt::RowWidgetInt(pqDynamicPropertiesWidget* parent, const QString& key,
  vtkDynamicProperties::Type type, const QString& description, int minValue, int maxValue,
  int defaultValue)
  : RowWidget(parent, key, type)
  , intRangeWidget(new pqIntRangeWidget(parent))
{
  this->intRangeWidget->setMinimum(minValue);
  this->intRangeWidget->setMaximum(maxValue);
  this->setValue(defaultValue);
  this->intRangeWidget->setToolTip(description);
  this->intRangeWidget->setProperty(keyPropertyName, key);

  this->layout->addWidget(this->intRangeWidget);

  parent->connect(this->intRangeWidget, SIGNAL(valueChanged(int)), SLOT(updateProperty()));
}

//------------------------------------------------------------------------------
QVariant RowWidgetInt::value()
{
  return QVariant(this->intRangeWidget->value());
}

//------------------------------------------------------------------------------
bool RowWidgetInt::setValue(const QVariant& value)
{
  bool ok;
  this->intRangeWidget->setValue(value.toInt(&ok));
  return ok;
}

//------------------------------------------------------------------------------
void RowWidgetInt::deleteLater()
{
  this->RowWidget::deleteLater();
  this->intRangeWidget->deleteLater();
}

//------------------------------------------------------------------------------
void RowWidgetInt::setValue(int value)
{
  bool oldBlock = this->intRangeWidget->blockSignals(true);
  this->intRangeWidget->setValue(value);
  this->intRangeWidget->blockSignals(oldBlock);
}

//------------------------------------------------------------------------------
class RowWidgetDouble : public RowWidget
{
public:
  RowWidgetDouble(pqDynamicPropertiesWidget* parent, const QString& key,
    vtkDynamicProperties::Type type, const QString& description, double minValue, double maxValue,
    double defaultValue);
  void deleteLater() override;
  QVariant value() override;
  bool setValue(const QVariant& value) override;
  void setValue(double);

private:
  pqDoubleRangeWidget* doubleRangeWidget;
};

//------------------------------------------------------------------------------
RowWidgetDouble::RowWidgetDouble(pqDynamicPropertiesWidget* parent, const QString& key,
  vtkDynamicProperties::Type type, const QString& description, double minValue, double maxValue,
  double defaultValue)
  : RowWidget(parent, key, type)
  , doubleRangeWidget(new pqDoubleRangeWidget(parent))
{
  this->doubleRangeWidget->setMinimum(minValue);
  this->doubleRangeWidget->setMaximum(maxValue);
  this->setValue(defaultValue);
  this->doubleRangeWidget->setToolTip(description);
  this->doubleRangeWidget->setProperty(keyPropertyName, key);

  this->layout->addWidget(this->doubleRangeWidget);

  parent->connect(this->doubleRangeWidget, SIGNAL(valueChanged(double)), SLOT(updateProperty()));
}

//------------------------------------------------------------------------------
QVariant RowWidgetDouble::value()
{
  return QVariant(this->doubleRangeWidget->value());
}

//------------------------------------------------------------------------------
bool RowWidgetDouble::setValue(const QVariant& value)
{
  bool ok;
  this->doubleRangeWidget->setValue(value.toDouble(&ok));
  return ok;
}

//------------------------------------------------------------------------------
void RowWidgetDouble::deleteLater()
{
  this->RowWidget::deleteLater();
  this->doubleRangeWidget->deleteLater();
}

//------------------------------------------------------------------------------
void RowWidgetDouble::setValue(double value)
{
  bool oldBlock = this->doubleRangeWidget->blockSignals(true);
  this->doubleRangeWidget->setValue(value);
  this->doubleRangeWidget->blockSignals(oldBlock);
}

} // end anon namespace

class pqDynamicPropertiesWidget::DomainModifiedObserver : public vtkCommand
{
public:
  // VTK macros for class instantiation
  static DomainModifiedObserver* New() { return new DomainModifiedObserver; }
  vtkTypeMacro(DomainModifiedObserver, vtkCommand);

  // The Execute method is called when the observed event occurs
  void Execute(vtkObject* caller, unsigned long eventId [[maybe_unused]],
    void* callData [[maybe_unused]]) override
  {
    auto* domain = vtkSMDynamicPropertiesDomain::SafeDownCast(caller);
    if (!domain)
    {
      qWarning() << Q_FUNC_INFO << "Expect caller to be a vtkSMDynamicPropertiesDomain.";
      return;
    }
    auto* pullProp = vtkSMStringVectorProperty::SafeDownCast(domain->GetInfoProperty());
    if (!pullProp)
    {
      qWarning() << Q_FUNC_INFO << "Expect property to be a vtkSMStringVectorProperty.";
      return;
    }
    this->Widget->buildWidget(pullProp);
  }
  pqDynamicPropertiesWidget* Widget = nullptr;
};

class pqDynamicPropertiesWidget::pqInternals
{
public:
  typedef QMap<QString, RowWidget*> WidgetMap;

  pqInternals() = default;

  ~pqInternals()
  {
    typedef WidgetMap::const_iterator Iter;
    const WidgetMap& map = this->widgetMap;
    for (Iter it = map.begin(), itEnd = map.end(); it != itEnd; ++it)
    {
      delete it.value();
    }
  }

  // If obj is an input widget tied to a particular dimension, return the
  // Widgets instance for that dimension. Handy for determining which dimension
  // triggered a slot execution with QObject::sender().
  RowWidget* findWidgets(QObject* obj)
  {
    if (obj)
    {
      QVariant keyVar = obj->property(keyPropertyName);
      if (keyVar.isValid())
      {
        return this->widgetMap.value(keyVar.toString(), nullptr);
      }
    }
    return nullptr;
  }

  WidgetMap widgetMap;
};

//------------------------------------------------------------------------------
pqDynamicPropertiesWidget::pqDynamicPropertiesWidget(
  vtkSMProxy* pxy, vtkSMProperty* pushProp, QWidget* parentW)
  : Superclass(pxy, parentW)
  , PropertyUpdatePending(false)
  , IgnorePushPropertyUpdates(false)
  , VBox(new QVBoxLayout(this))
  , GroupBox(
      new QGroupBox(QCoreApplication::translate("ServerManagerXML", pushProp->GetXMLLabel()), this))
  , Form(new QFormLayout(this->GroupBox))
  , Internals(new pqInternals())
{
  auto domain = pushProp->FindDomain<vtkSMDynamicPropertiesDomain>();
  if (!domain)
  {
    qWarning() << "Missing domain for index_selection widget.";
    return;
  }
  vtkSMProperty* pullProp = domain->GetInfoProperty();
  if (!pullProp)
  {
    qWarning() << "index_selection domain missing required 'Info' property.";
    return;
  }

  this->setPushPropertyName(pxy->GetPropertyName(pushProp));

  this->installEventFilter(this);

  this->GroupBox->setAlignment(Qt::AlignLeft);

  this->VBox->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  this->VBox->addWidget(this->GroupBox);

  this->Form->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  this->Form->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  this->Form->setLabelAlignment(Qt::AlignLeft);

  this->setShowLabel(false);

  this->buildWidget(pullProp);

  this->addPropertyLink(this, this->PushPropertyName, SIGNAL(widgetModified()), pushProp);

  if (this->Internals->widgetMap.empty())
  {
    this->hide();
  }

  vtkNew<DomainModifiedObserver> observer;
  observer->Widget = this;
  domain->AddObserver(vtkCommand::DomainModifiedEvent, observer);
}

//------------------------------------------------------------------------------
pqDynamicPropertiesWidget::~pqDynamicPropertiesWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::setHeaderLabel(const QString& str)
{
  this->GroupBox->setTitle(str);
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::setPushPropertyName(const QByteArray& pName)
{
  this->PushPropertyName = pName;
}

//------------------------------------------------------------------------------
bool pqDynamicPropertiesWidget::eventFilter(QObject* obj, QEvent* e)
{
  if (!this->PushPropertyName.isNull())
  {
    if (e->type() == QEvent::DynamicPropertyChange)
    {
      QDynamicPropertyChangeEvent* propEvent = static_cast<QDynamicPropertyChangeEvent*>(e);
      if (this->PushPropertyName == propEvent->propertyName() && !this->IgnorePushPropertyUpdates)
      {
        // Update the widget state when the PropertyLink updates the Qt
        // property:
        this->propertyChanged();
      }
    }
  }
  return this->Superclass::eventFilter(obj, e);
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::propertyChanged()
{
  QVariant propVar = this->property(this->PushPropertyName.data());
  QList<QVariant> prop = propVar.toList();
  if (prop.size() % 3 != 0)
  {
    qWarning() << Q_FUNC_INFO << "Invalid property list length.";
    return;
  }
  QString key = prop.at(0).toString();
  QString value = prop.at(2).toString();
  if (this->Internals->widgetMap.empty() && key == "" && value == "")
  {
    return;
  }

  bool ok;
  for (int i = 0; i < prop.size(); i += 3)
  {
    QString key = prop.at(i).toString();
    RowWidget* w = this->Internals->widgetMap.value(key, nullptr);
    if (!w)
    {
      qWarning() << Q_FUNC_INFO << "No widgets found for key" << key;
      continue;
    }
    w->type = static_cast<vtkDynamicProperties::Type>(prop.at(i + 1).toInt());
    ok = w->setValue(prop.at(i + 2));
    if (!ok)
    {
      qWarning() << Q_FUNC_INFO << "Cannot convert variant to index:" << prop.at(i + 1);
      continue;
    }
  }
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::updateProperty()
{
  if (this->PropertyUpdatePending)
  {
    return;
  }

  this->PropertyUpdatePending = true;
  QTimer::singleShot(250, this, SLOT(updatePropertyImpl()));
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::updatePropertyImpl()
{
  this->PropertyUpdatePending = false;
  QList<QVariant> newProp;

  // c'mon C++11 for-range and auto...
  typedef pqInternals::WidgetMap::const_iterator Iter;
  const pqInternals::WidgetMap& map = this->Internals->widgetMap;
  for (Iter it = map.begin(), itEnd = map.end(); it != itEnd; ++it)
  {
    auto* w = it.value();
    newProp.append(QVariant(it.key()));
    newProp.append(QVariant(w->type));
    newProp.append(w->value());
  }

  this->IgnorePushPropertyUpdates = true;
  this->setProperty(this->PushPropertyName.data(), newProp);
  this->IgnorePushPropertyUpdates = false;
  Q_EMIT widgetModified();
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::clearProperties()
{
  for (int i = 0; i < this->Form->rowCount(); ++i)
  {
    this->Form->takeRow(i);
  }
  auto& map = this->Internals->widgetMap;
  for (auto it = map.begin(); it != map.end(); ++it)
  {
    auto* widget = it.value();
    widget->deleteLater();
    delete widget;
  }
  map.clear();
}

//------------------------------------------------------------------------------
void pqDynamicPropertiesWidget::buildWidget(vtkSMProperty* infoProp)
{
  this->clearProperties();
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(infoProp);
  if (!svp)
  {
    qWarning() << Q_FUNC_INFO
               << "index_selection widget expects Hints/InfoProperty to be a "
                  "StringVectorProperty.";
    return;
  }

  std::string stringProperties{ svp->GetElement(0) };
  vtkLog(INFO, "ANARIRendererParametersInfo \n" << stringProperties);
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;

  // Parse from string
  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  if (!reader->parse(stringProperties.c_str(), stringProperties.c_str() + stringProperties.length(),
        &root, &errs))
  {
    qWarning() << "Error parsing parameters: " << errs;
    return;
  }
  if (root.isObject() && root.isMember(vtkDynamicProperties::PROPERTIES_KEY) &&
    root[vtkDynamicProperties::PROPERTIES_KEY].isArray())
  {
    Json::Value properties = root[vtkDynamicProperties::PROPERTIES_KEY];
    int row = 0;
    for (const auto& property : properties)
    {
      QString description =
        QString::fromStdString(property[vtkDynamicProperties::DESCRIPTION_KEY].asString());
      QString name = QString::fromStdString(property[vtkDynamicProperties::NAME_KEY].asString());
      vtkDynamicProperties::Type type =
        static_cast<vtkDynamicProperties::Type>(property[vtkDynamicProperties::TYPE_KEY].asInt());
      QLabel* pLabel = new QLabel(name);
      pLabel->setToolTip(description);
      RowWidget* rowWidget = nullptr;
      switch (type)
      {
        case vtkDynamicProperties::INT32:
        {
          if (!property.isMember(vtkDynamicProperties::DEFAULT_KEY))
          {
            qWarning() << name << " does not have a default value";
          }
          int defaultValue = property.isMember(vtkDynamicProperties::DEFAULT_KEY)
            ? property[vtkDynamicProperties::DEFAULT_KEY].asInt()
            : 1;
          int minVal = property.isMember(vtkDynamicProperties::MIN_KEY)
            ? property[vtkDynamicProperties::MIN_KEY].asInt()
            : defaultValue;
          int maxVal = property.isMember(vtkDynamicProperties::MAX_KEY)
            ? property[vtkDynamicProperties::MAX_KEY].asInt()
            : defaultValue;
          rowWidget = new RowWidgetInt(this, name, type, description, minVal, maxVal, defaultValue);
          break;
        }
        case vtkDynamicProperties::BOOL:
        {
          bool defaultValue = property.isMember(vtkDynamicProperties::DEFAULT_KEY)
            ? property[vtkDynamicProperties::DEFAULT_KEY].asBool()
            : false;
          rowWidget = new RowWidgetBool(this, name, type, description, defaultValue);
          break;
        }
        case vtkDynamicProperties::FLOAT32:
        {
          if (!property.isMember(vtkDynamicProperties::DEFAULT_KEY))
          {
            qWarning() << name << " does not have a default value";
          }
          double defaultValue = property.isMember(vtkDynamicProperties::DEFAULT_KEY)
            ? property[vtkDynamicProperties::DEFAULT_KEY].asFloat()
            : 1;
          double minVal = property.isMember(vtkDynamicProperties::MIN_KEY)
            ? property[vtkDynamicProperties::MIN_KEY].asFloat()
            : defaultValue;
          double maxVal = property.isMember(vtkDynamicProperties::MAX_KEY)
            ? property[vtkDynamicProperties::MAX_KEY].asFloat()
            : defaultValue;
          rowWidget =
            new RowWidgetDouble(this, name, type, description, minVal, maxVal, defaultValue);
          break;
        }
        default:
          break;
      }
      if (rowWidget)
      {
        this->Internals->widgetMap.insert(name, rowWidget);
        // Keep the list alphabetic:
        int row = this->findRow(name);
        this->Form->insertRow(row, rowWidget->label, rowWidget->layout);
      }
    }
  }

  // Force update the property
  this->updatePropertyImpl();
}

//------------------------------------------------------------------------------
int pqDynamicPropertiesWidget::findRow(const QString& key)
{
  int row = 0;
  for (; row < this->Form->rowCount(); ++row)
  {
    QLayoutItem* layoutItem = this->Form->itemAt(row, QFormLayout::LabelRole);
    if (layoutItem)
    {
      QLabel* label = qobject_cast<QLabel*>(layoutItem->widget());
      if (label)
      {
        if (key.compare(label->text(), Qt::CaseSensitive) < 0)
        {
          break;
        }
      }
    }
  }
  return row;
}
