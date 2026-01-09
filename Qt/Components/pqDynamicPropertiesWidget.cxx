// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqDynamicPropertiesWidget.h"
#include "pqDoubleRangeWidget.h"
#include "pqIntRangeWidget.h"
#include "pqPropertiesPanel.h"
#include "vtkDynamicProperties.h"
#include "vtkSMDynamicPropertiesDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include "vtkNew.h"
#include "json/json.h"

#include <QByteArray>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QDynamicPropertyChangeEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <iostream>
#include <limits>

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
  RowWidget(pqDynamicPropertiesWidget* parent, const QString& key, vtkDynamicProperties::Type type,
    const QString& description);
  virtual ~RowWidget();
  virtual void deleteLater();
  virtual QVariant value() = 0;
  virtual bool setValue(const QVariant& v) = 0;

  QHBoxLayout* Layout;
  QLabel* Label;
  QString Key;
  QString Description;
  vtkDynamicProperties::Type Type;
};

//------------------------------------------------------------------------------
RowWidget::RowWidget(pqDynamicPropertiesWidget* parent, const QString& k,
  vtkDynamicProperties::Type t, const QString& d)
  : Layout(new QHBoxLayout)
  , Label(new QLabel(k, parent))
  , Key(k)
  , Description(d)
  , Type(t)
{
  if (!Description.isEmpty())
  {
    Description[0] = Description[0].toUpper();
  }
  QString labelKey = Key;
  if (!labelKey.isEmpty())
  {
    labelKey[0] = labelKey[0].toUpper();
  }
  this->Label->setText(labelKey);
  this->Label->setToolTip(Description);
  this->Label->setProperty(keyPropertyName, Key);
  this->Layout->setContentsMargins(0, 0, 0, 0);
}

//------------------------------------------------------------------------------
RowWidget::~RowWidget()
{
  // It's possible for this to not have a parent set if the widget was not
  // added to an external layout, so ensure that it gets cleaned up.
  if (this->Layout)
  {
    this->Layout->deleteLater();
    this->Layout = nullptr;
  }
}

//------------------------------------------------------------------------------
void RowWidget::deleteLater()
{
  this->Label->deleteLater();
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
  QCheckBox* Checkbox;
};

//------------------------------------------------------------------------------
RowWidgetBool::RowWidgetBool(pqDynamicPropertiesWidget* parent, const QString& name,
  vtkDynamicProperties::Type type, const QString& d, bool state)
  : RowWidget(parent, name, type, d)
  , Checkbox(new QCheckBox(QString(), parent))
{
  this->setCheckState(state);
  this->Checkbox->setProperty(keyPropertyName, name);
  this->Checkbox->setToolTip(Description);
  this->Layout->addWidget(this->Checkbox);
  parent->connect(
    this->Checkbox, SIGNAL(checkStateChanged(Qt::CheckState)), SLOT(updateProperty()));
}

//------------------------------------------------------------------------------
QVariant RowWidgetBool::value()
{
  return QVariant(this->Checkbox->checkState() == Qt::Checked);
}

//------------------------------------------------------------------------------
bool RowWidgetBool::setValue(const QVariant& value)
{
  this->Checkbox->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
  return true;
}

//------------------------------------------------------------------------------
void RowWidgetBool::deleteLater()
{
  this->RowWidget::deleteLater();
  this->Checkbox->deleteLater();
}

//------------------------------------------------------------------------------
void RowWidgetBool::setCheckState(bool state)
{
  bool oldBlock = this->Checkbox->blockSignals(true);
  this->Checkbox->setCheckState(state ? Qt::Checked : Qt::Unchecked);
  this->Checkbox->blockSignals(oldBlock);
}

//------------------------------------------------------------------------------
template <typename NumberWidgetType, typename NumberType>
class RowWidgetNumber : public RowWidget
{
public:
  RowWidgetNumber(pqDynamicPropertiesWidget* parent, const QString& key,
    vtkDynamicProperties::Type type, const QString& description, NumberType minValue,
    NumberType maxValue, NumberType defaultValue);
  void deleteLater() override;
  QVariant value() override;
  bool setValue(const QVariant& value) override;
  void setValue(NumberType value);

private:
  NumberWidgetType* NumberWidget;
};

//------------------------------------------------------------------------------
template <typename NumberWidgetType, typename NumberType>
RowWidgetNumber<NumberWidgetType, NumberType>::RowWidgetNumber(pqDynamicPropertiesWidget* parent,
  const QString& key, vtkDynamicProperties::Type type, const QString& d, NumberType minValue,
  NumberType maxValue, NumberType defaultValue)
  : RowWidget(parent, key, type, d)
  , NumberWidget(new NumberWidgetType(parent))
{
  this->NumberWidget->setMinimum(minValue);
  this->NumberWidget->setMaximum(maxValue);
  QString tooltip = Description;
  if (minValue != std::numeric_limits<NumberType>::lowest())
  {
    tooltip += (". min=" + QString::number(minValue));
  }
  if (maxValue != std::numeric_limits<NumberType>::max())
  {
    tooltip += (", max=" + QString::number(maxValue));
  }
  this->setValue(defaultValue);
  this->NumberWidget->setToolTip(tooltip);
  this->NumberWidget->setProperty(keyPropertyName, key);

  this->Layout->addWidget(this->NumberWidget);
  if constexpr (std::is_same_v<NumberType, int>)
  {
    parent->connect(this->NumberWidget, SIGNAL(valueChanged(int)), SLOT(updateProperty()));
  }
  else if constexpr (std::is_same_v<NumberType, double>)
  {
    parent->connect(this->NumberWidget, SIGNAL(valueChanged(double)), SLOT(updateProperty()));
  }
}

//------------------------------------------------------------------------------
template <typename NumberWidgetType, typename NumberType>
QVariant RowWidgetNumber<NumberWidgetType, NumberType>::value()
{
  return QVariant(this->NumberWidget->value());
}

//------------------------------------------------------------------------------
template <typename NumberWidgetType, typename NumberType>
bool RowWidgetNumber<NumberWidgetType, NumberType>::setValue(const QVariant& value)
{
  bool ok;
  if constexpr (std::is_same_v<NumberType, int>)
  {
    this->NumberWidget->setValue(value.toInt(&ok));
  }
  else if constexpr (std::is_same_v<NumberType, double>)
  {
    this->NumberWidget->setValue(value.toDouble(&ok));
  }
  return ok;
}

//------------------------------------------------------------------------------
template <typename NumberWidgetType, typename NumberType>
void RowWidgetNumber<NumberWidgetType, NumberType>::deleteLater()
{
  this->RowWidget::deleteLater();
  this->NumberWidget->deleteLater();
}

//------------------------------------------------------------------------------
template <typename NumberWidgetType, typename NumberType>
void RowWidgetNumber<NumberWidgetType, NumberType>::setValue(NumberType value)
{
  bool oldBlock = this->NumberWidget->blockSignals(true);
  this->NumberWidget->setValue(value);
  this->NumberWidget->blockSignals(oldBlock);
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
  vtkSMDomain* Domain = nullptr;
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
  vtkNew<DomainModifiedObserver> observer;
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

  this->Internals->observer->Widget = this;
  this->Internals->observer->Domain = domain;
  domain->AddObserver(vtkCommand::DomainModifiedEvent, this->Internals->observer);
}

//------------------------------------------------------------------------------
pqDynamicPropertiesWidget::~pqDynamicPropertiesWidget()
{
  this->Internals->observer->Domain->RemoveObserver(this->Internals->observer);
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
  QString key0 = prop.at(0).toString();
  QString value0 = prop.at(2).toString();
  if (this->Internals->widgetMap.empty() && key0 == "" && value0 == "")
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
    w->Type = static_cast<vtkDynamicProperties::Type>(prop.at(i + 1).toInt());
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
    newProp.append(QVariant(w->Type));
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
  for (int i = this->Form->rowCount() - 1; i >= 0; --i)
  {
    this->Form->takeRow(i);
  }
  assert(this->Form->rowCount() == 0);
  auto& map = this->Internals->widgetMap;
  for (auto it = map.begin(); it != map.end(); ++it)
  {
    auto* widget = it.value();
    widget->deleteLater();
    delete widget;
  }
  map.clear();
  assert(this->Internals->widgetMap.empty());
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
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;

  // Parse from string
  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  if (!reader->parse(stringProperties.c_str(), stringProperties.c_str() + stringProperties.length(),
        &root, &errs))
  {
    qWarning() << "Error parsing parameters: " << errs.c_str();
    return;
  }
  if (root.isObject() && root.isMember(vtkDynamicProperties::PROPERTIES_KEY) &&
    root[vtkDynamicProperties::PROPERTIES_KEY].isArray())
  {
    if (!root.isMember(vtkDynamicProperties::VERSION_KEY))
    {
      qWarning() << "Error: No version provided with the properties "
                    "json specification";
      return;
    }
    if (root[vtkDynamicProperties::VERSION_KEY].asInt() <
      VTK_DYNAMIC_PROPERTIES_VERSION_NUMBER_QUICK)
    {
      qWarning() << "Error: json properties specification version "
                    "is not supported: "
                 << root[vtkDynamicProperties::VERSION_KEY].asInt()
                 << " expecting: " << VTK_DYNAMIC_PROPERTIES_VERSION_NUMBER_QUICK;
      return;
    }
    Json::Value properties = root[vtkDynamicProperties::PROPERTIES_KEY];
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
        case vtkDynamicProperties::INT:
        {
          if (!property.isMember(vtkDynamicProperties::DEFAULT_KEY))
          {
            qWarning() << name << " does not have a default value";
          }
          int defaultValue = property.isMember(vtkDynamicProperties::DEFAULT_KEY)
            ? property[vtkDynamicProperties::DEFAULT_KEY].asInt()
            : 1;
          if (property.isMember(vtkDynamicProperties::MAX_KEY) &&
            property.isMember(vtkDynamicProperties::MIN_KEY))
          {
            int minVal = property[vtkDynamicProperties::MIN_KEY].asInt();
            int maxVal = property[vtkDynamicProperties::MAX_KEY].asInt();
            rowWidget = new RowWidgetNumber<pqIntRangeWidget, int>(
              this, name, type, description, minVal, maxVal, defaultValue);
          }
          else
          {
            int minVal = property.isMember(vtkDynamicProperties::MIN_KEY)
              ? property[vtkDynamicProperties::MIN_KEY].asInt()
              : std::numeric_limits<int>::lowest();
            int maxVal = property.isMember(vtkDynamicProperties::MAX_KEY)
              ? property[vtkDynamicProperties::MAX_KEY].asInt()
              : std::numeric_limits<int>::max();
            rowWidget = new RowWidgetNumber<QSpinBox, int>(
              this, name, type, description, minVal, maxVal, defaultValue);
          }
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
        case vtkDynamicProperties::DOUBLE:
        {
          if (!property.isMember(vtkDynamicProperties::DEFAULT_KEY))
          {
            qWarning() << name << " does not have a default value";
          }
          double defaultValue = property.isMember(vtkDynamicProperties::DEFAULT_KEY)
            ? property[vtkDynamicProperties::DEFAULT_KEY].asDouble()
            : 1;
          if (property.isMember(vtkDynamicProperties::MIN_KEY) &&
            property.isMember(vtkDynamicProperties::MAX_KEY))
          {
            double minVal = property[vtkDynamicProperties::MIN_KEY].asDouble();
            double maxVal = property[vtkDynamicProperties::MAX_KEY].asDouble();
            rowWidget = new RowWidgetNumber<pqDoubleRangeWidget, double>(
              this, name, type, description, minVal, maxVal, defaultValue);
          }
          else
          {
            double minVal = property.isMember(vtkDynamicProperties::MIN_KEY)
              ? property[vtkDynamicProperties::MIN_KEY].asDouble()
              : std::numeric_limits<double>::lowest();
            double maxVal = property.isMember(vtkDynamicProperties::MAX_KEY)
              ? property[vtkDynamicProperties::MAX_KEY].asDouble()
              : std::numeric_limits<double>::max();
            rowWidget = new RowWidgetNumber<QDoubleSpinBox, double>(
              this, name, type, description, minVal, maxVal, defaultValue);
          }
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
        this->Form->insertRow(row, rowWidget->Label, rowWidget->Layout);
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
  QString labelKey(key);
  if (!labelKey.isEmpty())
  {
    labelKey[0] = labelKey[0].toUpper();
  }
  for (; row < this->Form->rowCount(); ++row)
  {
    QLayoutItem* layoutItem = this->Form->itemAt(row, QFormLayout::LabelRole);
    if (layoutItem)
    {
      QLabel* label = qobject_cast<QLabel*>(layoutItem->widget());
      if (label)
      {
        if (labelKey.compare(label->text(), Qt::CaseSensitive) < 0)
        {
          break;
        }
      }
    }
  }
  return row;
}
