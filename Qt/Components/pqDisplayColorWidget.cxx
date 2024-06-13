// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDisplayColorWidget.h"

#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "pqScalarsToColors.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QStandardItemModel>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <cassert>
#include <vector>

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
/// (all done by vtkSMColorMapEditorHelper::SetSelectedScalarColoring()).
class pqDisplayColorWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
private:
  using Superclass = pqPropertyLinksConnection;
  using ValueType = pqDisplayColorWidget::ValueType;

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = nullptr)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
    this->ColorMapEditorHelper->SetSelectedPropertiesType(
      vtkSMColorMapEditorHelper::GetPropertyType(smproperty));
  }
  ~PropertyLinksConnection() override = default;

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
    const QStringList strList = value.toStringList();
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

    const ValueType val = this->convert(value);
    const int association = val.first;
    const QString& arrayName = val.second;

    const QString undoText = QCoreApplication::translate("PropertyLinksConnection", "Change ") +
      QCoreApplication::translate("ServerManagerXML", this->propertySM()->GetXMLLabel());
    BEGIN_UNDO_SET(undoText);
    vtkSMProxy* reprProxy = this->proxySM();

    // before changing scalar color, save the LUT being used so we can hide the
    // scalar bar later, if needed.
    const std::vector<vtkSMProxy*> oldLutProxies =
      this->ColorMapEditorHelper->GetSelectedLookupTables(reprProxy);

    this->ColorMapEditorHelper->SetSelectedScalarColoring(
      reprProxy, arrayName.toUtf8().data(), association);

    vtkSMViewProxy* view = qobject_cast<pqDisplayColorWidget*>(this->objectQt())->viewProxy();

    // Hide unused scalar bars, if applicable.
    pqDisplayColorWidget::hideScalarBarsIfNotNeeded(view, oldLutProxies);

    if (!arrayName.isEmpty())
    {
      pqDisplayColorWidget::updateScalarBarVisibility(
        view, reprProxy, this->ColorMapEditorHelper->GetSelectedPropertiesType());
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
    if (this->ColorMapEditorHelper->GetAnySelectedUsingScalarColoring(reprProxy))
    {
      const auto colorArrays = this->ColorMapEditorHelper->GetSelectedColorArrays(reprProxy);
      if (!colorArrays.empty())
      {
        // Always get the first array.
        val.first = colorArrays[0].first;
        val.second = colorArrays[0].second.c_str();
      }
    }
    return this->convert(val);
  }

  /// called to set the UI due to a ServerManager Property change.
  void setQtValue(const QVariant& value) override
  {
    const ValueType val = this->convert(value);
    qobject_cast<pqDisplayColorWidget*>(this->objectQt())->setArraySelection(val);
  }

  /// called to get the UI value.
  QVariant currentQtValue() const override
  {
    const ValueType curVal =
      qobject_cast<pqDisplayColorWidget*>(this->objectQt())->arraySelection();
    return this->convert(curVal);
  }

private:
  Q_DISABLE_COPY(PropertyLinksConnection)
};

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::hideScalarBarsIfNotNeeded(
  vtkSMViewProxy* view, std::vector<vtkSMProxy*> luts)
{
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  vtkPVGeneralSettings* generalSettings = vtkPVGeneralSettings::GetInstance();
  for (auto lut : luts)
  {
    switch (generalSettings->GetScalarBarMode())
    {
      case vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS:
      case vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS:
        tmgr->HideScalarBarIfNotNeeded(lut, view);
        break;
      default:
        break;
    }
  }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::updateScalarBarVisibility(
  vtkSMViewProxy* view, vtkSMProxy* reprProxy, int selectedPropertiesType)
{
  vtkNew<vtkSMColorMapEditorHelper> colorMapEditorHelper;
  colorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
  // we could now respect some application setting to determine if the LUT is to be reset.
  colorMapEditorHelper->RescaleSelectedTransferFunctionToDataRange(
    reprProxy, /*extend*/ true, /*force*/ false);

  /// BUG #0011858. Users often do silly things!
  const bool reprVisibility =
    vtkSMPropertyHelper(reprProxy, "Visibility", /*quiet*/ true).GetAsInt() == 1;

  vtkPVGeneralSettings* gsettings = vtkPVGeneralSettings::GetInstance();

  // now show used scalar bars if applicable.
  if (reprVisibility &&
    gsettings->GetScalarBarMode() == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
  {
    colorMapEditorHelper->SetSelectedScalarBarVisibility(reprProxy, view, true);
  }
}

//=============================================================================
class pqDisplayColorWidget::pqInternals : public QObject
{
public:
  pqPropertyLinks Links;
  vtkWeakPointer<vtkSMArrayListDomain> Domain;

  unsigned long DomainObserverId = 0;

  int OutOfDomainEntryIndex;
  QSet<QString> NoSolidColor;

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;

  pqInternals()
    : OutOfDomainEntryIndex(-1)
  {
  }
};

//-----------------------------------------------------------------------------
pqDisplayColorWidget::pqDisplayColorWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqDisplayColorWidget::pqInternals())
  , CellDataIcon(new QIcon(":/pqWidgets/Icons/pqCellData.svg"))
  , PointDataIcon(new QIcon(":/pqWidgets/Icons/pqPointData.svg"))
  , FieldDataIcon(new QIcon(":/pqWidgets/Icons/pqGlobalData.svg"))
  , SolidColorIcon(new QIcon(":/pqWidgets/Icons/pqSolidColor.svg"))
{
  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setContentsMargins(pqPropertiesPanel::suggestedMargins());
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

  QObject::connect(this->Variables, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
    this->refreshComponents();
    this->pruneOutOfDomainEntries();
    Q_EMIT this->arraySelectionChanged();
  });
  QObject::connect(this->Components, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqDisplayColorWidget::componentNumberChanged);

  QObject::connect(&this->Internals->Links, &pqPropertyLinks::qtWidgetChanged, this,
    &pqDisplayColorWidget::renderActiveView);

  // The representation type, like Slice or Volume, can disable some entries.
  QObject::connect(this, &pqDisplayColorWidget::representationTextChanged, this,
    &pqDisplayColorWidget::refreshColorArrayNames);
}

//-----------------------------------------------------------------------------
pqDisplayColorWidget::~pqDisplayColorWidget() = default;

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
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setRepresentation(pqDataRepresentation* repr, int selectedPropertiesType)
{
  if (this->Representation != nullptr && this->Representation == repr &&
    this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType() == selectedPropertiesType)
  {
    return;
  }

  if (this->Representation)
  {
    this->disconnect(this->Representation);
    this->Representation = nullptr;
  }
  if (this->ColorTransferFunction)
  {
    this->disconnect(this->ColorTransferFunction);
    this->ColorTransferFunction->disconnect(this);
    this->ColorTransferFunction = nullptr;
  }
  this->Variables->setEnabled(false);
  this->Components->setEnabled(false);
  this->Internals->Links.clear();
  this->Internals->NoSolidColor.clear();
  if (this->Internals->DomainObserverId != 0 && this->Internals->Domain)
  {
    this->Internals->Domain->GetProperty()->RemoveObserver(this->Internals->DomainObserverId);
    this->Internals->DomainObserverId = 0;
  }
  this->Internals->Domain = nullptr;
  this->Variables->clear();
  this->Components->clear();
  this->Internals->OutOfDomainEntryIndex = -1;

  // now, save the new repr.
  this->Representation = repr;
  this->Internals->ColorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);

  vtkSMProxy* proxy = repr ? repr->getProxy() : nullptr;
  vtkSMProperty* colorArrayProp =
    proxy ? this->Internals->ColorMapEditorHelper->GetSelectedColorArrayProperty(proxy) : nullptr;
  const bool canColor = (proxy != nullptr && colorArrayProp != nullptr);
  if (!canColor)
  {
    return;
  }

  auto domain = colorArrayProp->FindDomain<vtkSMArrayListDomain>();
  if (!domain)
  {
    qCritical("%s",
      QString("Representation has %1 property without "
              "a vtkSMArrayListDomain. This is no longer supported.")
        .arg(colorArrayProp->GetXMLName())
        .toUtf8()
        .data());
    return;
  }
  this->Internals->Domain = domain;
  this->Internals->DomainObserverId = pqCoreUtilities::connect(
    colorArrayProp, vtkCommand::DomainModifiedEvent, this, SLOT(refreshColorArrayNames()));
  this->refreshColorArrayNames();

  // monitor LUT changes. we need to link the component number on the LUT's property.
  if (this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType() ==
    vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks)
  {
    QObject::connect(repr, &pqDataRepresentation::blockColorTransferFunctionModified, this,
      &pqDisplayColorWidget::updateColorTransferFunction);
  }
  else
  {
    QObject::connect(repr, &pqDataRepresentation::colorTransferFunctionModified, this,
      &pqDisplayColorWidget::updateColorTransferFunction);
  }

  this->Internals->Links.addPropertyLink<pqDisplayColorWidget::PropertyLinksConnection>(
    this, "notused", SIGNAL(arraySelectionChanged()), proxy, colorArrayProp);
  // add a link to representation, because some representations don't like Solid Color
  vtkSMProperty* reprProperty = proxy->GetProperty("Representation");
  if (reprProperty)
  {
    this->Internals->Links.addPropertyLink(this, "representationText", "", proxy, reprProperty);
    // process hints to see which representation types skip "Solid Color".
    vtkPVXMLElement* hints = proxy->GetHints();
    for (unsigned int cc = 0; cc < (hints ? hints->GetNumberOfNestedElements() : 0); cc++)
    {
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "NoSolidColor") == 0)
      {
        this->Internals->NoSolidColor.insert(child->GetAttributeOrEmpty("representation"));
      }
    }
  }
  this->updateColorTransferFunction();
}

//-----------------------------------------------------------------------------
QPair<int, QString> pqDisplayColorWidget::arraySelection() const
{
  if (this->Variables->currentIndex() != -1)
  {
    return pqDisplayColorWidget::PropertyLinksConnection::convert(
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

  const QVariant idata = this->itemData(association, arrayName);
  int index = this->Variables->findData(idata);
  if (index == -1)
  {
    // NOTE: it is possible in several occasions (e.g. when domain is not
    // up-to-date, or property value is explicitly set to something not in the
    // domain) that the selected array is not present in the domain. In that
    // case, based on what we know of the array selection, we update the UI and
    // add that entry to the UI.
    const bool prev = this->Variables->blockSignals(true);
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
  return pqDisplayColorWidget::PropertyLinksConnection::convert(ValueType(association, arrayName));
}

//-----------------------------------------------------------------------------
QIcon* pqDisplayColorWidget::itemIcon(int association, const QString& arrayName) const
{
  const QVariant idata = this->itemData(association, arrayName);
  if (!idata.isValid())
  {
    return this->SolidColorIcon.get();
  }
  if (association == vtkDataObject::FIELD)
  {
    return this->FieldDataIcon.get();
  }
  else if (association == vtkDataObject::CELL)
  {
    return this->CellDataIcon.get();
  }
  else
  {
    return this->PointDataIcon.get();
  }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::queryCurrentSelectedArray()
{
  if (this->Representation && this->Internals->Links.getPropertyLink(0))
  {
    // force the current selected array to be queried.
    Q_EMIT this->Internals->Links.getPropertyLink(0)->smpropertyModified();
    // ensure that we are showing the correct selected component
    this->updateColorTransferFunction();
  }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::refreshColorArrayNames()
{
  // Simply update the this->Variables combo-box with values from the domain.
  // Try to preserve the current text if possible.
  const QVariant current =
    pqDisplayColorWidget::PropertyLinksConnection::convert(this->arraySelection());

  const bool prev = this->Variables->blockSignals(true);

  this->Variables->clear();
  this->Internals->OutOfDomainEntryIndex = -1;

  // add solid color, but disable for some representations.
  this->Variables->addItem(*this->SolidColorIcon, tr("Solid Color"), QVariant());
  if (this->Representation && this->Internals->NoSolidColor.contains(this->representationText()))
  {
    const auto model = qobject_cast<QStandardItemModel*>(this->Variables->model());
    QStandardItem* item = model->item(0);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
  }

  vtkSMArrayListDomain* domain = this->Internals->Domain;
  assert(domain);
  for (unsigned int cc = 0, max = domain->GetNumberOfStrings(); cc < max; cc++)
  {
    const int icon_association = domain->GetDomainAssociation(cc);
    const int association = domain->GetFieldAssociation(cc);
    const QString name = domain->GetString(cc);
    const QString label = !domain->IsArrayPartial(cc) ? name : QString("%1 (partial)").arg(name);
    const QIcon* icon = this->itemIcon(icon_association, name);
    const QVariant idata = this->itemData(association, name);
    const QString key = QString("%1.%2").arg(association).arg(name);
    this->Variables->addItem(*icon, label, idata);
  }
  // doing this here, instead of in the constructor ensures that the
  // popup menu shows reasonably on OsX.
  this->Variables->setMaxVisibleItems(this->Variables->count() + 1);

  const int idx = this->Variables->findData(current);
  if (idx > 0)
  {
    this->Variables->setCurrentIndex(idx);
  }
  else if (current.isValid())
  {
    // The previous value set on the widget is no longer available in the
    // "domain". This happens when the pipeline is modified, for example. In
    // that case, the UI still needs to reflect the value. So we add it.
    const QPair<int, QString> currentColoring =
      pqDisplayColorWidget::PropertyLinksConnection::convert(current);
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
  const int index = this->Components->currentIndex();
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
      qDebug() << "Component " << val << " is not currently known. Will add a new entry for it.";
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
    BEGIN_UNDO_SET(tr("Change Color Component"));
    const int number = this->componentNumber();
    const QPair<int, QString> val = this->arraySelection();
    const int association = val.first;
    const QString& arrayName = val.second;

    const int vectorMode = number < 0 ? pqScalarsToColors::MAGNITUDE : pqScalarsToColors::COMPONENT;
    const int vectorComponent =
      vectorMode == pqScalarsToColors::COMPONENT ? std::max(number, 0) : 0;

    vtkSMProxy* repr = this->Representation ? this->Representation->getProxy() : nullptr;

    const std::vector<vtkSMProxy*> luts =
      this->Internals->ColorMapEditorHelper->GetSelectedLookupTables(repr);
    for (const auto& lut : luts)
    {
      // pqScalarsToColors::setVectorMode
      vtkSMPropertyHelper(lut, "VectorMode").Set(vectorMode);
      vtkSMPropertyHelper(lut, "VectorComponent").Set(vectorComponent);
      lut->UpdateVTKObjects();
    }

    if (this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType() ==
      vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Representation)
    {
      SM_SCOPED_TRACE(SetScalarColoring)
        .arg("display", repr)
        .arg("arrayname", arrayName.toUtf8().data())
        .arg("attribute_type", association)
        .arg("component", this->Components->itemText(number + 1).toUtf8().data())
        .arg("lut", luts.front());
    }
    else
    {
      SM_SCOPED_TRACE(SetBlocksScalarColoring)
        .arg("display", repr)
        .arg(
          "block_selectors", this->Internals->ColorMapEditorHelper->GetSelectedBlockSelectors(repr))
        .arg("arrayname", arrayName.toUtf8().data())
        .arg("attribute_type", association)
        .arg("component", this->Components->itemText(number + 1).toUtf8().data())
        .arg("luts", std::vector<vtkObject*>({ luts.begin(), luts.end() }));
    }
    // we could now respect some application setting to determine if the LUT is to be reset.
    this->Internals->ColorMapEditorHelper->RescaleSelectedTransferFunctionToDataRange(
      repr, /*extend*/ false, /*force*/ false);

    // Update scalar bars.
    vtkNew<vtkSMTransferFunctionManager> tmgr;
    for (const auto& lut : luts)
    {
      tmgr->UpdateScalarBarsComponentTitle(lut, repr);
    }
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

  const int compNumber = this->componentNumber();
  const QPair<int, QString> val = this->arraySelection();
  const int association = val.first;
  const QString& arrayName = val.second;

  vtkPVDataInformation* dataInfo = this->Representation->getInputDataInformation();
  vtkPVArrayInformation* arrayInfo =
    dataInfo ? dataInfo->GetArrayInformation(arrayName.toUtf8().data(), association) : nullptr;
  if (!arrayInfo)
  {
    vtkPVDataInformation* reprInfo = this->Representation->getRepresentedDataInformation();
    arrayInfo =
      reprInfo ? reprInfo->GetArrayInformation(arrayName.toUtf8().data(), association) : nullptr;
  }

  if (!arrayInfo || arrayInfo->GetNumberOfComponents() <= 1)
  {
    // using solid color or coloring by non-existing array.
    this->Components->setEnabled(false);
    const bool prev = this->Components->blockSignals(true);
    this->Components->clear();
    this->Components->blockSignals(prev);
    return;
  }

  const bool prev = this->Components->blockSignals(true);

  const int nComponents = arrayInfo->GetNumberOfComponents();
  this->Components->clear();

  // add magnitude for non-scalar values when needed
  auto arraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  if (nComponents > 1 && arraySettings->ShouldUseMagnitudeMode(nComponents))
  {
    this->Components->addItem(arrayInfo->GetComponentName(-1), -1);
  }

  for (int comp = 0; comp < nComponents; comp++)
  {
    this->Components->addItem(arrayInfo->GetComponentName(comp), comp);
  }

  /// restore component choice if possible.
  const int idx = this->Components->findData(compNumber);
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

  pqScalarsToColors* lut = this->Representation->getLookupTable(
    this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType());
  if (this->ColorTransferFunction && this->ColorTransferFunction != lut)
  {
    this->disconnect(this->ColorTransferFunction);
    this->ColorTransferFunction->disconnect(this);
    this->ColorTransferFunction = nullptr;
  }
  if (lut)
  {
    this->ColorTransferFunction = lut;
    this->setComponentNumber(
      lut->getVectorMode() == pqScalarsToColors::MAGNITUDE ? -1 : lut->getVectorComponent());
    QObject::connect(lut, &pqScalarsToColors::componentOrModeChanged, this,
      &pqDisplayColorWidget::updateColorTransferFunction, Qt::UniqueConnection);
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

void pqDisplayColorWidget::setRepresentationText(const QString& text)
{
  if (this->RepresentationText != text)
  {
    this->RepresentationText = text;
    // this lets us disable Solid Color based on the representation type changing.
    Q_EMIT this->representationTextChanged(text);
  }
}
