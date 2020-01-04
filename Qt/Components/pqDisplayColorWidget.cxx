/*=========================================================================

   Program: ParaView
   Module:    pqDisplayColorWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqDisplayColorWidget.h"

#include "pqDataRepresentation.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqScalarsToColors.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QtDebug>

#include <cassert>

namespace
{

// Returns true if assoc is a field association that is supported by this class.
bool supportedAssociation(int assoc)
{
  switch (assoc)
  {
    case vtkDataObject::POINT:
    case vtkDataObject::CELL:
    case vtkDataObject::FIELD:
    case vtkDataObject::VERTEX:
      return true;

    default:
      break;
  }
  return false;
}

} // end anon namespace

//=============================================================================
/// This class makes it possible to add custom logic when updating the
/// "ColorArrayName" property instead of directly setting the SMProperty. The
/// custom logic, in this case, ensures that the LUT is setup and initialized
/// (all done by vtkSMPVRepresentationProxy::SetScalarColoring()).
class pqDisplayColorWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;
  typedef pqDisplayColorWidget::ValueType ValueType;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = 0)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }
  ~PropertyLinksConnection() override {}

  // Methods to convert between ValueType and QVariant. Since QVariant doesn't
  // support == operations of non-default Qt types, we are forced to convert the
  // ValueType to a QStringList.
  static ValueType convert(const QVariant& value)
  {
    if (!value.isValid())
    {
      return ValueType();
    }
    assert(value.canConvert<QStringList>());
    QStringList strList = value.toStringList();
    assert(strList.size() == 2);
    return ValueType(strList[0].toInt(), strList[1]);
  }
  static QVariant convert(const ValueType& value)
  {
    if (!supportedAssociation(value.first) || value.second.isEmpty())
    {
      return QVariant();
    }

    QStringList val;
    val << QString::number(value.first) << value.second;
    return val;
  }

protected:
  /// Called to update the ServerManager Property due to UI change.
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    assert(use_unchecked == false);
    Q_UNUSED(use_unchecked);

    ValueType val = this->convert(value);
    int association = val.first;
    const QString& arrayName = val.second;

    BEGIN_UNDO_SET("Change coloring");
    vtkSMProxy* reprProxy = this->proxySM();

    // before changing scalar color, save the LUT being used so we can hide the
    // scalar bar later, if needed.
    vtkSMProxy* oldLutProxy = vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();

    vtkSMPVRepresentationProxy::SetScalarColoring(
      reprProxy, arrayName.toLocal8Bit().data(), association);

    vtkNew<vtkSMTransferFunctionManager> tmgr;
    pqDisplayColorWidget* widget = qobject_cast<pqDisplayColorWidget*>(this->objectQt());
    vtkSMViewProxy* view = widget->viewProxy();

    // Hide unused scalar bars, if applicable.
    vtkPVGeneralSettings* gsettings = vtkPVGeneralSettings::GetInstance();
    switch (gsettings->GetScalarBarMode())
    {
      case vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS:
      case vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS:
        tmgr->HideScalarBarIfNotNeeded(oldLutProxy, view);
        break;
    }

    if (!arrayName.isEmpty())
    {
      // we could now respect some application setting to determine if the LUT is
      // to be reset.
      vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
        reprProxy, /*extend*/ true, /*force*/ false);

      /// BUG #0011858. Users often do silly things!
      bool reprVisibility =
        vtkSMPropertyHelper(reprProxy, "Visibility", /*quiet*/ true).GetAsInt() == 1;

      // now show used scalar bars if applicable.
      if (reprVisibility &&
        gsettings->GetScalarBarMode() ==
          vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
      {
        vtkSMPVRepresentationProxy::SetScalarBarVisibility(reprProxy, view, true);
      }
    }
    END_UNDO_SET();
  }

  /// called to get the current value for the ServerManager Property.
  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    assert(use_unchecked == false);
    Q_UNUSED(use_unchecked);
    ValueType val;
    vtkSMProxy* reprProxy = this->proxySM();
    if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy))
    {
      vtkSMPropertyHelper helper(this->propertySM());
      val.first = helper.GetInputArrayAssociation();
      val.second = helper.GetInputArrayNameToProcess();
    }
    return this->convert(val);
  }

  /// called to set the UI due to a ServerManager Property change.
  void setQtValue(const QVariant& value) override
  {
    ValueType val = this->convert(value);
    qobject_cast<pqDisplayColorWidget*>(this->objectQt())->setArraySelection(val);
  }

  /// called to get the UI value.
  QVariant currentQtValue() const override
  {
    ValueType curVal = qobject_cast<pqDisplayColorWidget*>(this->objectQt())->arraySelection();
    return this->convert(curVal);
  }

private:
  Q_DISABLE_COPY(PropertyLinksConnection)
};

//=============================================================================
class pqDisplayColorWidget::pqInternals
{
public:
  pqPropertyLinks Links;
  vtkNew<vtkEventQtSlotConnect> Connector;
  vtkWeakPointer<vtkSMArrayListDomain> Domain;
  int OutOfDomainEntryIndex;

  pqInternals()
    : OutOfDomainEntryIndex(-1)
  {
  }
};

//-----------------------------------------------------------------------------
pqDisplayColorWidget::pqDisplayColorWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , CachedRepresentation(NULL)
  , Internals(new pqDisplayColorWidget::pqInternals())
{
  this->CellDataIcon = new QIcon(":/pqWidgets/Icons/pqCellData.svg");
  this->PointDataIcon = new QIcon(":/pqWidgets/Icons/pqPointData.svg");
  this->FieldDataIcon = new QIcon(":/pqWidgets/Icons/pqGlobalData.svg");
  this->SolidColorIcon = new QIcon(":/pqWidgets/Icons/pqSolidColor.svg");

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setMargin(0);
  hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  this->Variables = new QComboBox(this);
  // this->Variables->setMaxVisibleItems(60);
  this->Variables->setObjectName("Variables");
  this->Variables->setMinimumSize(QSize(150, 0));
  this->Variables->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  this->Components = new QComboBox(this);
  this->Components->setObjectName("Components");

  hbox->addWidget(this->Variables);
  hbox->addWidget(this->Components);

  this->Variables->setEnabled(false);
  this->Components->setEnabled(false);

  this->connect(this->Variables, SIGNAL(currentIndexChanged(int)), SLOT(refreshComponents()));
  this->connect(this->Variables, SIGNAL(currentIndexChanged(int)), SLOT(pruneOutOfDomainEntries()));
  this->connect(this->Variables, SIGNAL(currentIndexChanged(int)), SIGNAL(arraySelectionChanged()));
  this->connect(this->Components, SIGNAL(currentIndexChanged(int)), SLOT(componentNumberChanged()));

  this->connect(&this->Internals->Links, SIGNAL(qtWidgetChanged()), SLOT(renderActiveView()));
}

//-----------------------------------------------------------------------------
pqDisplayColorWidget::~pqDisplayColorWidget()
{
  delete this->FieldDataIcon;
  delete this->CellDataIcon;
  delete this->PointDataIcon;
  delete this->SolidColorIcon;
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::renderActiveView()
{
  if (this->Representation)
  {
    this->Representation->renderViewEventually();
  }
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqDisplayColorWidget::viewProxy() const
{
  if (this->Representation)
  {
    return this->Representation->getViewProxy();
  }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setRepresentation(pqDataRepresentation* repr)
{
  if (this->CachedRepresentation == repr)
  {
    return;
  }

  if (this->Representation)
  {
    this->disconnect(this->Representation);
    this->Representation = NULL;
  }
  if (this->ColorTransferFunction)
  {
    this->disconnect(this->ColorTransferFunction);
    this->ColorTransferFunction->disconnect(this);
    this->ColorTransferFunction = NULL;
  }
  this->Variables->setEnabled(false);
  this->Components->setEnabled(false);
  this->Internals->Links.clear();
  this->Internals->Connector->Disconnect();
  this->Internals->Domain = NULL;
  this->Variables->clear();
  this->Components->clear();
  this->Internals->OutOfDomainEntryIndex = -1;

  // now, save the new repr.
  this->CachedRepresentation = repr;
  this->Representation = repr;

  vtkSMProxy* proxy = repr ? repr->getProxy() : NULL;
  bool can_color = (proxy != NULL && proxy->GetProperty("ColorArrayName") != NULL);
  if (!can_color)
  {
    return;
  }

  vtkSMProperty* prop = proxy->GetProperty("ColorArrayName");
  auto domain = prop->FindDomain<vtkSMArrayListDomain>();
  if (!domain)
  {
    qCritical("Representation has ColorArrayName property without "
              "a vtkSMArrayListDomain. This is no longer supported.");
    return;
  }
  this->Internals->Domain = domain;

  this->Internals->Connector->Connect(
    prop, vtkCommand::DomainModifiedEvent, this, SLOT(refreshColorArrayNames()));
  this->refreshColorArrayNames();

  // monitor LUT changes. we need to link the component number on the LUT's
  // property.
  this->connect(repr, SIGNAL(colorTransferFunctionModified()), SLOT(updateColorTransferFunction()));

  this->Internals->Links.addPropertyLink<PropertyLinksConnection>(
    this, "notused", SIGNAL(arraySelectionChanged()), proxy, prop);

  this->updateColorTransferFunction();
}

//-----------------------------------------------------------------------------
QPair<int, QString> pqDisplayColorWidget::arraySelection() const
{
  if (this->Variables->currentIndex() != -1)
  {
    return PropertyLinksConnection::convert(
      this->Variables->itemData(this->Variables->currentIndex()));
  }

  return QPair<int, QString>();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setArraySelection(const QPair<int, QString>& value)
{
  int association = value.first;
  const QString& arrayName = value.second;

  if (!supportedAssociation(association))
  {
    qCritical("Unsupported field association.");
    association = vtkDataObject::POINT;
  }

  QVariant idata = this->itemData(association, arrayName);
  int index = this->Variables->findData(idata);
  if (index == -1)
  {
    // NOTE: it is possible in several occasions (e.g. when domain is not
    // up-to-date, or property value is explicitly set to something not in the
    // domain) that the selected array is not present in the domain. In that
    // case, based on what we know of the array selection, we update the UI and
    // add that entry to the UI.
    bool prev = this->Variables->blockSignals(true);
    index = this->addOutOfDomainEntry(association, arrayName);
    this->Variables->blockSignals(prev);
  }
  assert(index != -1);
  this->Variables->setCurrentIndex(index);
  this->refreshComponents();
}

//-----------------------------------------------------------------------------
QVariant pqDisplayColorWidget::itemData(int association, const QString& arrayName) const
{
  return PropertyLinksConnection::convert(ValueType(association, arrayName));
}

//-----------------------------------------------------------------------------
QIcon* pqDisplayColorWidget::itemIcon(int association, const QString& arrayName) const
{
  QVariant idata = this->itemData(association, arrayName);
  if (!idata.isValid())
  {
    return this->SolidColorIcon;
  }
  if (association == vtkDataObject::FIELD)
  {
    return this->FieldDataIcon;
  }
  else if (association == vtkDataObject::CELL)
  {
    return this->CellDataIcon;
  }
  else
  {
    return this->PointDataIcon;
  }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::refreshColorArrayNames()
{
  // Simply update the this->Variables combo-box with values from the domain.
  // Try to preserve the current text if possible.
  QVariant current = PropertyLinksConnection::convert(this->arraySelection());

  bool prev = this->Variables->blockSignals(true);

  this->Variables->clear();
  this->Internals->OutOfDomainEntryIndex = -1;

  // add solid color.
  this->Variables->addItem(*this->SolidColorIcon, "Solid Color", QVariant());

  vtkSMArrayListDomain* domain = this->Internals->Domain;
  assert(domain);

  QSet<QString> uniquifier; // we sometimes end up with duplicate entries.
  // overcome that for now. We need a proper fix in
  // vtkSMArrayListDomain/vtkSMRepresentedArrayListDomain/vtkPVDataInformation.
  // for now, just do this. To reproduce the problem, skip the uniquifier check,
  // and the load can.ex2, uncheck all blocks, and check all sets. The color-by
  // combo will have duplicated ObjectId, SourceElementId, and SourceElementSide entries.
  for (unsigned int cc = 0, max = domain->GetNumberOfStrings(); cc < max; cc++)
  {
    int icon_association = domain->GetDomainAssociation(cc);
    int association = domain->GetFieldAssociation(cc);
    QString name = domain->GetString(cc);
    QString label = name;
    if (domain->IsArrayPartial(cc))
    {
      label += " (partial)";
    }
    QIcon* icon = this->itemIcon(icon_association, name);
    QVariant idata = this->itemData(association, name);
    QString key = QString("%1.%2").arg(association).arg(name);
    if (!uniquifier.contains(key))
    {
      this->Variables->addItem(*icon, label, idata);
      uniquifier.insert(key);
    }
  }
  // doing this here, instead of in the constructor ensures that the
  // popup menu shows resonably on OsX.
  this->Variables->setMaxVisibleItems(this->Variables->count() + 1);

  int idx = this->Variables->findData(current);
  if (idx > 0)
  {
    this->Variables->setCurrentIndex(idx);
  }
  else if (current.isValid())
  {
    // The previous value set on the widget is no longer available in the
    // "domain". This happens when the pipeline is modified, for example. In
    // that case, the UI still needs to reflect the value. So we add it.
    QPair<int, QString> currentColoring = PropertyLinksConnection::convert(current);
    this->Variables->setCurrentIndex(
      this->addOutOfDomainEntry(currentColoring.first, currentColoring.second));
  }
  this->Variables->blockSignals(prev);
  this->Variables->setEnabled(this->Variables->count() > 0);

  // since a change in domain could mean a change in components too,
  // we should refresh those as well.
  this->refreshComponents();

  // now, refreshing the list of components is not enough. We also need to
  // ensure that we are showing the correct selected component. Calling
  // updateColorTransferFunction() ensures that widget is updated to reflect the
  // current component number selection, if applicable.
  this->updateColorTransferFunction();
}

//-----------------------------------------------------------------------------
int pqDisplayColorWidget::componentNumber() const
{
  // here, we treat -1 as the magnitude (to be consistent with
  // VTK/vtkScalarsToColors). This mismatches with how
  // vtkPVArrayInformation refers to magnitude.
  int index = this->Components->currentIndex();
  return index >= 0 ? this->Components->itemData(index).toInt() : -1;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setComponentNumber(int val)
{
  if (this->Components->isEnabled())
  {
    int index = this->Components->findData(val);
    if (index == -1)
    {
      this->Components->blockSignals(true);
      this->Components->addItem(QString("%1").arg(val), val);
      this->Components->blockSignals(false);

      index = this->Components->findData(val);
      qDebug() << "Component " << val << " is not currently known. "
                                         "Will add a new entry for it.";
    }
    assert(index != -1);
    this->Components->blockSignals(true);
    this->Components->setCurrentIndex(index);
    this->Components->blockSignals(false);
  }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::componentNumberChanged()
{
  if (this->ColorTransferFunction)
  {
    BEGIN_UNDO_SET("Change color component");
    int number = this->componentNumber();
    QPair<int, QString> val = this->arraySelection();
    int association = val.first;
    const QString& arrayName = val.second;
    this->ColorTransferFunction->setVectorMode(
      number < 0 ? pqScalarsToColors::MAGNITUDE : pqScalarsToColors::COMPONENT,
      number < 0 ? 0 : number);
    { // To make sure the trace is done before other calls.
      SM_SCOPED_TRACE(SetScalarColoring)
        .arg("display", this->Representation->getProxy())
        .arg("arrayname", arrayName.toLatin1().data())
        .arg("attribute_type", association)
        .arg("component", this->Components->itemText(number + 1).toLatin1().data())
        .arg("lut", this->ColorTransferFunction->getProxy());
    }

    // we could now respect some application setting to determine if the LUT is
    // to be reset.
    vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : NULL;
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
      reprProxy, /*extend*/ false, /*force*/ false);

    // Update scalar bars.
    vtkNew<vtkSMTransferFunctionManager> tmgr;
    tmgr->UpdateScalarBarsComponentTitle(this->ColorTransferFunction->getProxy(), reprProxy);

    END_UNDO_SET();

    // render all views since this could affect multiple views.
    pqApplicationCore::instance()->render();
  }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::refreshComponents()
{
  if (!this->Representation)
  {
    return;
  }

  int comp_number = this->componentNumber();

  QPair<int, QString> val = this->arraySelection();
  int association = val.first;
  const QString& arrayName = val.second;

  vtkPVDataInformation* dataInfo = this->Representation->getInputDataInformation();
  vtkPVArrayInformation* arrayInfo =
    dataInfo ? dataInfo->GetArrayInformation(arrayName.toLocal8Bit().data(), association) : NULL;
  if (!arrayInfo)
  {
    vtkPVDataInformation* reprInfo = this->Representation->getRepresentedDataInformation();
    arrayInfo =
      reprInfo ? reprInfo->GetArrayInformation(arrayName.toLocal8Bit().data(), association) : NULL;
  }

  if (!arrayInfo || arrayInfo->GetNumberOfComponents() <= 1)
  {
    // using solid color or coloring by non-existing array.
    this->Components->setEnabled(false);
    bool prev = this->Components->blockSignals(true);
    this->Components->clear();
    this->Components->blockSignals(prev);
    return;
  }

  bool prev = this->Components->blockSignals(true);

  this->Components->clear();
  for (int cc = 0, max = arrayInfo->GetNumberOfComponents(); cc < max; cc++)
  {
    if (cc == 0 && max > 1)
    {
      // add magnitude for non-scalar values.
      this->Components->addItem(arrayInfo->GetComponentName(-1), -1);
    }
    this->Components->addItem(arrayInfo->GetComponentName(cc), cc);
  }

  /// restore component choice if possible.
  int idx = this->Components->findData(comp_number);
  if (idx != -1)
  {
    this->Components->setCurrentIndex(idx);
  }
  this->Components->blockSignals(prev);
  this->Components->setEnabled(this->Components->count() > 0);
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::updateColorTransferFunction()
{
  if (!this->Representation)
  {
    return;
  }

  pqScalarsToColors* lut = this->Representation->getLookupTable();
  if (this->ColorTransferFunction && this->ColorTransferFunction != lut)
  {
    this->disconnect(this->ColorTransferFunction);
    this->ColorTransferFunction->disconnect(this);
  }
  this->ColorTransferFunction = lut;
  if (lut)
  {
    this->setComponentNumber(
      lut->getVectorMode() == pqScalarsToColors::MAGNITUDE ? -1 : lut->getVectorComponent());
    this->connect(lut, SIGNAL(componentOrModeChanged()), SLOT(updateColorTransferFunction()),
      Qt::UniqueConnection);
  }
}

//-----------------------------------------------------------------------------
int pqDisplayColorWidget::addOutOfDomainEntry(int association, const QString& arrayName)
{
  this->Internals->OutOfDomainEntryIndex = this->Variables->count();
  // typically, we'd ask the domain to tell us if this is an array being
  // "auto-converted" and if so, we'll use that icon type. But we don't have
  // that information anymore. So just use the association indicated on the
  // property.
  this->Variables->addItem(*this->itemIcon(association, arrayName), arrayName + " (?)",
    this->itemData(association, arrayName));
  return this->Internals->OutOfDomainEntryIndex;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::pruneOutOfDomainEntries()
{
  if (this->Internals->OutOfDomainEntryIndex != -1 &&
    this->Variables->currentIndex() != this->Internals->OutOfDomainEntryIndex)
  {
    // Yay! Prune that entry!
    this->Variables->removeItem(this->Internals->OutOfDomainEntryIndex);
    this->Internals->OutOfDomainEntryIndex = -1;
  }
}
