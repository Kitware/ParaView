// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDisplayRepresentationWidget.h"
#include "ui_pqDisplayRepresentationWidget.h"

#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqDisplayColorWidget.h"
#include "pqPropertyLinks.h"
#include "pqUndoStack.h"
#include "vtkPVXMLElement.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMViewProxy.h"

#include <QPointer>
#include <QSet>

#include <cassert>

//=============================================================================
class pqDisplayRepresentationWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = nullptr)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }
  ~PropertyLinksConnection() override = default;

protected:
  /// Called to update the ServerManager Property due to UI change.
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    assert(use_unchecked == false);
    Q_UNUSED(use_unchecked);

    BEGIN_UNDO_SET(
      QCoreApplication::translate("PropertyLinksConnection", "Change representation type"));
    vtkSMProxy* reprProxy = this->proxySM();
    auto widget = qobject_cast<pqDisplayRepresentationWidget*>(this->objectQt());
    vtkSMViewProxy* view = widget->viewProxy();
    if (auto reprPVProxy = vtkSMPVRepresentationProxy::SafeDownCast(reprProxy))
    {
      // In case we might volume render,
      // we're looking at wether we need to update the scalar bar visibility.
      // We want to update the scalar bar visibility if no LUT proxy exists.
      // (it means that the user clicked on `Volume` while no array was selected)
      // This needs to be done before calling SetRepresentationType, as this method
      // will setup a LUT proxy.
      vtkSMProxy* lutProxy = vtkSMColorMapEditorHelper::GetLookupTable(reprPVProxy, view);
      const QString& type = value.toString();

      // Let'set the new representation
      reprPVProxy->SetRepresentationType(type.toUtf8().data());

      // When volume rendering, we need to set a scalar bar if it was not already present,
      // because volume redering can only be done when using a scalar field.
      if (!lutProxy && type == QString("Volume"))
      {
        pqDisplayColorWidget::updateScalarBarVisibility(
          view, reprProxy, vtkSMColorMapEditorHelper::Representation);
      }
    }
    END_UNDO_SET();
  }

private:
  Q_DISABLE_COPY(PropertyLinksConnection)
};

//=============================================================================
class pqDisplayRepresentationWidget::pqInternals : public Ui::displayRepresentationWidget
{
  QString RepresentationText;

public:
  pqPropertyLinks Links;
  QPointer<pqComboBoxDomain> Domain;
  QPointer<pqDataRepresentation> PQRepr;
  QSet<QString> WarnOnRepresentationChange;
  pqInternals() = default;

  bool setRepresentationText(const QString& text)
  {
    int idx = this->comboBox->findText(text);
    if (idx != -1)
    {
      bool prev = this->comboBox->blockSignals(true);
      this->comboBox->setCurrentIndex(idx);
      this->RepresentationText = text;
      this->comboBox->blockSignals(prev);
    }
    return (idx != -1);
  }

  const QString& representationText() const { return this->RepresentationText; }
};

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::pqDisplayRepresentationWidget(QWidget* _p)
  : Superclass(_p)
{
  this->Internal = new pqDisplayRepresentationWidget::pqInternals();
  this->Internal->setupUi(this);
  QObject::connect(this->Internal->comboBox, &QComboBox::currentTextChanged, this,
    &pqDisplayRepresentationWidget::comboBoxChanged);
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::~pqDisplayRepresentationWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqDisplayRepresentationWidget::viewProxy() const
{
  return this->Representation ? this->Representation->getViewProxy() : nullptr;
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentation(pqDataRepresentation* display)
{
  if (this->Internal->PQRepr)
  {
    this->Internal->PQRepr->disconnect(this);
  }
  vtkSMProxy* proxy = display ? display->getProxy() : nullptr;
  this->setRepresentation(proxy);
  this->Internal->PQRepr = display;
  if (display)
  {
    display->connect(
      this, SIGNAL(representationTextChanged(const QString&)), SLOT(renderViewEventually()));
  }

  this->Representation = display;
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentation(vtkSMProxy* proxy)
{
  // break old links.
  this->Internal->Links.clear();
  this->Internal->WarnOnRepresentationChange.clear();
  delete this->Internal->Domain;
  bool prev = this->Internal->comboBox->blockSignals(true);
  this->Internal->comboBox->clear();
  vtkSMProperty* smproperty = proxy ? proxy->GetProperty("Representation") : nullptr;
  this->Internal->comboBox->setEnabled(smproperty != nullptr);
  if (!smproperty)
  {
    this->Internal->comboBox->addItem(tr("Representation"));
    this->Internal->comboBox->blockSignals(prev);
    return;
  }

  this->Internal->Domain = new pqComboBoxDomain(this->Internal->comboBox, smproperty);
  this->Internal->Links.addPropertyLink<PropertyLinksConnection>(this, "representationText",
    SIGNAL(representationTextChanged(const QString&)), proxy, smproperty);
  this->Internal->comboBox->blockSignals(prev);

  // process hints to see which representation types we need to warn the user
  // about.
  vtkPVXMLElement* hints = proxy->GetHints();
  for (unsigned int cc = 0; cc < (hints ? hints->GetNumberOfNestedElements() : 0); cc++)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "WarnOnRepresentationChange") == 0)
    {
      this->Internal->WarnOnRepresentationChange.insert(child->GetAttributeOrEmpty("value"));
    }
  }
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentationText(const QString& text)
{
  this->Internal->setRepresentationText(text);
}

//-----------------------------------------------------------------------------
QString pqDisplayRepresentationWidget::representationText() const
{
  return this->Internal->comboBox->isEnabled() ? this->Internal->representationText() : QString();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::comboBoxChanged(const QString& text)
{
  // NOTE: this method doesn't get called when
  // pqDisplayRepresentationWidget::setRepresentationText() is called.
  if (this->Internal->WarnOnRepresentationChange.contains(text))
  {
    bool confirmed =
      pqCoreUtilities::promptUser(QString("pqDisplayRepresentationWidget_type_%1").arg(text),
        QMessageBox::Question, "Are you sure?",
        QString("This will change the representation type to \"%1\".\n"
                "That may take a while, depending on your dataset.\n"
                "Are you sure?")
          .arg(text),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Save);

    if (!confirmed)
    {
      this->Internal->setRepresentationText(this->Internal->representationText());
      return;
    }
  }
  this->Internal->setRepresentationText(text);
  Q_EMIT this->representationTextChanged(text);
}

//=============================================================================
pqDisplayRepresentationPropertyWidget::pqDisplayRepresentationPropertyWidget(
  vtkSMProxy* smProxy, QWidget* parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  QVBoxLayout* layoutLocal = new QVBoxLayout;
  layoutLocal->setContentsMargins(0, 0, 0, 0);
  this->Widget = new pqDisplayRepresentationWidget(this);
  layoutLocal->addWidget(this->Widget);
  setLayout(layoutLocal);
  this->Widget->setRepresentation(smProxy);

  this->connect(
    this->Widget, SIGNAL(representationTextChanged(const QString&)), SIGNAL(changeAvailable()));
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationPropertyWidget::~pqDisplayRepresentationPropertyWidget() = default;
