/*=========================================================================

   Program: ParaView
   Module: pqIndexSelectionWidget

   Copyright (c) 2015 Sandia Corporation, Kitware Inc.
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

#include "pqIndexSelectionWidget.h"

#include "pqLineEdit.h"
#include "pqPropertiesPanel.h"
#include "pqSMAdaptor.h"

#include "vtkSMIndexSelectionDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include "vtkNew.h"
#include "vtkStringList.h"

#include <QByteArray>
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

#include <algorithm>

namespace
{

// Unsurprisingly, this function clamps val between min and max.
template <typename T>
inline T clamp(const T& val, const T& min, const T& max)
{
  return std::min(max, std::max(min, val));
}

//------------------------------------------------------------------------------
// Used to name Qt properties on widgets so that they can be mapped back to a
// key (used with QObject::sender() from slots).
const char keyPropertyName[] = "IndexSelectionKey";

//------------------------------------------------------------------------------
// Internal helper class to hold and configure a single dimension's widgets.
struct Widgets
{
  Widgets()
    : layout(NULL)
    , label(NULL)
    , slider(NULL)
    , edit(NULL)
  {
  }
  Widgets(pqIndexSelectionWidget* parent, const QString& key, int current, int size);
  ~Widgets();

  void setCurrent(int current);
  void setCurrentSlider(int current);
  void setCurrentEdit(int current);
  void setSize(int size);

  QHBoxLayout* layout;
  QLabel* label;
  QSlider* slider;
  pqLineEdit* edit;
};

//------------------------------------------------------------------------------
Widgets::Widgets(pqIndexSelectionWidget* parent, const QString& key, int current_, int size_)
  : layout(new QHBoxLayout)
  , label(new QLabel(key, parent))
  , slider(new QSlider(Qt::Horizontal, parent))
  , edit(new pqLineEdit(parent))
{
  this->setSize(size_);
  this->setCurrent(current_);

  this->slider->setObjectName("Slider");
  this->edit->setObjectName("LineEdit");

  this->label->setProperty(keyPropertyName, key);
  this->slider->setProperty(keyPropertyName, key);
  this->edit->setProperty(keyPropertyName, key);

  this->layout->setMargin(0);
  this->layout->addWidget(this->slider);
  this->layout->addWidget(this->edit);

  this->edit->setValidator(new QIntValidator(this->edit));

  parent->connect(this->slider, SIGNAL(valueChanged(int)), SLOT(currentChanged(int)));
  parent->connect(
    this->edit, SIGNAL(textChanged(const QString&)), SLOT(currentChanged(const QString&)));
  parent->connect(this->edit, SIGNAL(textChangedAndEditingFinished()), SLOT(currentChanged()));

  parent->connect(this->slider, SIGNAL(valueChanged(int)), SLOT(updateProperty()));
  parent->connect(this->edit, SIGNAL(textChanged(const QString&)), SLOT(updateProperty()));
}

//------------------------------------------------------------------------------
Widgets::~Widgets()
{
  // It's possible for this to not have a parent set if the widget was not
  // added to an external layout, so ensure that it gets cleaned up.
  if (this->layout)
  {
    this->layout->deleteLater();
    this->layout = NULL;
  }
}

//------------------------------------------------------------------------------
void Widgets::setCurrent(int cur)
{
  this->setCurrentSlider(cur);
  this->setCurrentEdit(cur);
}

//------------------------------------------------------------------------------
void Widgets::setCurrentSlider(int cur)
{
  cur = clamp(cur, this->slider->minimum(), this->slider->maximum());
  bool oldBlock = this->slider->blockSignals(true);
  this->slider->setValue(cur);
  this->slider->blockSignals(oldBlock);
}

//------------------------------------------------------------------------------
void Widgets::setCurrentEdit(int cur)
{
  cur = clamp(cur, this->slider->minimum(), this->slider->maximum());
  bool oldBlock = this->edit->blockSignals(true);
  this->edit->setTextAndResetCursor(QString::number(cur));
  this->edit->blockSignals(oldBlock);
}

//------------------------------------------------------------------------------
void Widgets::setSize(int sz)
{
  bool oldBlock = this->slider->blockSignals(true);
  this->slider->setRange(0, sz - 1);
  this->slider->blockSignals(oldBlock);
}

} // end anon namespace

class pqIndexSelectionWidget::pqInternals
{
public:
  typedef QMap<QString, Widgets*> WidgetMap;

  pqInternals() {}

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
  Widgets* findWidgets(QObject* obj)
  {
    if (obj)
    {
      QVariant keyVar = obj->property(keyPropertyName);
      if (keyVar.isValid())
      {
        return this->widgetMap.value(keyVar.toString(), NULL);
      }
    }
    return NULL;
  }

  WidgetMap widgetMap;
};

//------------------------------------------------------------------------------
pqIndexSelectionWidget::pqIndexSelectionWidget(
  vtkSMProxy* pxy, vtkSMProperty* pushProp, QWidget* parentW)
  : Superclass(pxy, parentW)
  , PropertyUpdatePending(false)
  , IgnorePushPropertyUpdates(false)
  , GroupBox(new QGroupBox(QString(pushProp->GetXMLLabel()), this))
  , VBox(new QVBoxLayout(this))
  , Form(new QFormLayout(this->GroupBox))
  , Internals(new pqInternals())
{
  auto domain = pushProp->FindDomain<vtkSMIndexSelectionDomain>();
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

  this->VBox->setMargin(pqPropertiesPanel::suggestedMargin());
  this->VBox->addWidget(this->GroupBox);

  this->Form->setMargin(pqPropertiesPanel::suggestedMargin());
  this->Form->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  this->Form->setLabelAlignment(Qt::AlignLeft);

  this->setShowLabel(false);

  this->buildWidget(pullProp);

  this->addPropertyLink(this, this->PushPropertyName, SIGNAL(widgetModified()), pushProp);

  if (this->Internals->widgetMap.empty())
  {
    this->hide();
  }
}

//------------------------------------------------------------------------------
pqIndexSelectionWidget::~pqIndexSelectionWidget()
{
  delete this->Internals;
  this->Internals = 0;
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::setHeaderLabel(const QString& str)
{
  this->GroupBox->setTitle(str);
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::setPushPropertyName(const QByteArray& pName)
{
  this->PushPropertyName = pName;
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::currentChanged(const QString& current)
{
  bool ok;
  int val = current.toInt(&ok);
  if (ok)
  {
    Widgets* widgets = this->Internals->findWidgets(this->sender());
    if (widgets)
    {
      // This is a live update during a text edit, so only update the slider:
      widgets->setCurrentSlider(val);
    }
  }
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::currentChanged(int current)
{
  Widgets* widgets = this->Internals->findWidgets(this->sender());
  if (widgets)
  {
    widgets->setCurrent(current);
  }
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::currentChanged()
{
  // The pqLineEdit::textChangedAndEditingFinished signal doesn't include the
  // current text. Look it up and adjust the slider.
  Widgets* widgets = this->Internals->findWidgets(this->sender());
  if (widgets)
  {
    bool ok;
    int val = widgets->edit->text().toInt(&ok);
    if (ok)
    {
      widgets->setCurrent(val);
    }
  }
}

//------------------------------------------------------------------------------
bool pqIndexSelectionWidget::eventFilter(QObject* obj, QEvent* e)
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
void pqIndexSelectionWidget::propertyChanged()
{
  QVariant propVar = this->property(this->PushPropertyName.constData());
  QList<QVariant> prop = propVar.toList();
  if (prop.size() % 2 != 0)
  {
    qWarning() << Q_FUNC_INFO << "Invalid property list length.";
    return;
  }

  bool ok;
  for (int i = 0; i < prop.size(); i += 2)
  {
    QString key = prop.at(i).toString();
    int idx = prop.at(i + 1).toInt(&ok);
    if (!ok)
    {
      qWarning() << Q_FUNC_INFO << "Cannot convert variant to index:" << prop.at(i + 1);
      continue;
    }

    Widgets* w = this->Internals->widgetMap.value(key, NULL);
    if (!w)
    {
      qWarning() << Q_FUNC_INFO << "No widgets found for key" << key;
      continue;
    }

    w->setCurrent(idx);
  }
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::updateProperty()
{
  if (this->PropertyUpdatePending)
  {
    return;
  }

  this->PropertyUpdatePending = true;
  QTimer::singleShot(250, this, SLOT(updatePropertyImpl()));
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::updatePropertyImpl()
{
  this->PropertyUpdatePending = false;
  QList<QVariant> newProp;

  // c'mon C++11 for-range and auto...
  typedef pqInternals::WidgetMap::const_iterator Iter;
  const pqInternals::WidgetMap& map = this->Internals->widgetMap;
  for (Iter it = map.begin(), itEnd = map.end(); it != itEnd; ++it)
  {
    newProp.append(QVariant(it.key()));
    newProp.append(QVariant(QString::number(it.value()->slider->value())));
  }

  this->IgnorePushPropertyUpdates = true;
  this->setProperty(this->PushPropertyName.constData(), newProp);
  this->IgnorePushPropertyUpdates = false;
  Q_EMIT widgetModified();
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::buildWidget(vtkSMProperty* infoProp)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(infoProp);
  if (!svp)
  {
    qWarning() << Q_FUNC_INFO << "index_selection widget expects "
                                 "Hints/InfoProperty to be a "
                                 "StringVectorProperty.";
    return;
  }

  vtkNew<vtkStringList> strings;
  svp->GetElements(strings.GetPointer());
  if (strings->GetNumberOfStrings() % 3 != 0)
  {
    qWarning() << Q_FUNC_INFO << "index_selection InfoProperty size must be a "
                                 "multiple of 3.";
    return;
  }

  bool ok;
  for (int i = 0; i < strings->GetNumberOfStrings(); i += 3)
  {
    QString key(strings->GetString(i));
    int cur = QByteArray(strings->GetString(i + 1)).toInt(&ok);
    if (!ok)
    {
      qWarning() << "Error parsing index info: invalid current index" << strings->GetString(i + 1);
      continue;
    }
    int sz = QByteArray(strings->GetString(i + 2)).toInt(&ok);
    if (!ok)
    {
      qWarning() << "Error parsing index info: invalid size" << strings->GetString(i + 2);
      continue;
    }

    this->addRow(key, cur, sz);
  }

  // Force update the property
  this->updatePropertyImpl();
}

//------------------------------------------------------------------------------
void pqIndexSelectionWidget::addRow(const QString& key, int current, int sz)
{
  Widgets* widgets = new Widgets(this, key, current, sz);
  this->Internals->widgetMap.insert(key, widgets);

  // Keep the list alphabetic:
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

  this->Form->insertRow(row, widgets->label, widgets->layout);
}
