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
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QtDebug>


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
  PropertyLinksConnection(
    QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex,
    bool use_unchecked_modified_event,
    QObject* parentObject=0)
    : Superclass(qobject, qproperty, qsignal,
      smproxy, smproperty, smindex, use_unchecked_modified_event,
      parentObject)
    {
    }
  virtual ~PropertyLinksConnection()
    {
    }

  // Methods to convert between ValueType and QVariant. Since QVariant doesn't
  // support == operations of non-default Qt types, we are forced to convert the
  // ValueType to a QStringList.
  static ValueType convert(const QVariant& value)
    {
    if (!value.isValid()) { return ValueType(); }
    Q_ASSERT(value.canConvert<QStringList>());
    QStringList strList = value.toStringList();
    Q_ASSERT(strList.size() == 2);
    return ValueType(strList[0].toInt(), strList[1]);
    }
  static QVariant convert(const ValueType& value)
    {
    if (value.first < vtkDataObject::POINT || value.first > vtkDataObject::CELL ||
      value.second.isEmpty())
      {
      return QVariant();
      }
    QStringList val;
    val << QString::number(value.first)
        << value.second;
    return val;
    }

protected:
  /// Called to update the ServerManager Property due to UI change.
  virtual void setServerManagerValue(bool use_unchecked, const QVariant& value)
    {
    Q_ASSERT(use_unchecked == false);

    ValueType val = this->convert(value);
    int association = val.first;
    const QString &arrayName = val.second;

    BEGIN_UNDO_SET("Change coloring");
    vtkSMProxy* reprProxy = this->proxySM();

    // before changing scalar color, save the LUT being used so we can hide the
    // scalar bar later, if needed.
    vtkSMProxy* oldLutProxy = vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();

    vtkSMPVRepresentationProxy::SetScalarColoring(reprProxy, arrayName.toLatin1().data(), association);

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
      vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(reprProxy, true);

      // now show used scalar bars if applicable.
      if (gsettings->GetScalarBarMode() ==
        vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
        {
        vtkSMPVRepresentationProxy::SetScalarBarVisibility(reprProxy, view, true);
        }
      }
    END_UNDO_SET();
    }

  /// called to get the current value for the ServerManager Property.
  virtual QVariant currentServerManagerValue(bool use_unchecked) const
    {
    Q_ASSERT(use_unchecked == false);
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
  virtual void setQtValue(const QVariant& value)
    {
    ValueType val = this->convert(value);
    qobject_cast<pqDisplayColorWidget*>(this->objectQt())->setArraySelection(val);
    }

  /// called to get the UI value.
  virtual QVariant currentQtValue() const
    {
    ValueType curVal = qobject_cast<pqDisplayColorWidget*>(
      this->objectQt())->arraySelection();
    return this->convert(curVal);
    }
private:
  Q_DISABLE_COPY(PropertyLinksConnection);
};

//=============================================================================
class pqDisplayColorWidget::pqInternals
{
public:
  pqPropertyLinks Links;
  vtkNew<vtkEventQtSlotConnect> Connector;
  vtkWeakPointer<vtkSMArrayListDomain> Domain;
};

//-----------------------------------------------------------------------------
pqDisplayColorWidget::pqDisplayColorWidget( QWidget *parentObject ) :
  Superclass(parentObject),
  CachedRepresentation(NULL),
  Internals(new pqDisplayColorWidget::pqInternals())
{
  this->CellDataIcon = new QIcon(":/pqWidgets/Icons/pqCellData16.png");
  this->PointDataIcon = new QIcon(":/pqWidgets/Icons/pqPointData16.png");
  this->SolidColorIcon = new QIcon(":/pqWidgets/Icons/pqSolidColor16.png");

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setMargin(0);
  hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  this->Variables = new QComboBox( this );
  //this->Variables->setMaxVisibleItems(60);
  this->Variables->setObjectName("Variables");
  this->Variables->setMinimumSize( QSize( 150, 0 ) );
  this->Variables->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  this->Components = new QComboBox( this );
  this->Components->setObjectName("Components");

  hbox->addWidget(this->Variables);
  hbox->addWidget(this->Components);

  this->Variables->setEnabled(false);
  this->Components->setEnabled(false);

  this->connect(this->Variables,
    SIGNAL(currentIndexChanged(int)), SLOT(refreshComponents()));
  this->connect(this->Variables,
    SIGNAL(currentIndexChanged(int)), SIGNAL(arraySelectionChanged()));
  this->connect(this->Components,
    SIGNAL(currentIndexChanged(int)), SLOT(componentNumberChanged()));

  this->connect(&this->Internals->Links,
    SIGNAL(qtWidgetChanged()), SLOT(renderActiveView()));
}

//-----------------------------------------------------------------------------
pqDisplayColorWidget::~pqDisplayColorWidget()
{
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

  // now, save the new repr.
  this->CachedRepresentation = repr;
  this->Representation = repr;

  vtkSMProxy* proxy = repr? repr->getProxy() : NULL;
  bool can_color = (proxy != NULL && proxy->GetProperty("ColorArrayName") != NULL);
  if (!can_color)
    {
    return;
    }

  vtkSMProperty* prop = proxy->GetProperty("ColorArrayName");
  vtkSMArrayListDomain* domain = vtkSMArrayListDomain::SafeDownCast(
    prop->FindDomain("vtkSMArrayListDomain"));
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
  this->connect(repr, SIGNAL(colorTransferFunctionModified()),
    SLOT(updateColorTransferFunction()));

  this->Internals->Links.addPropertyLink<PropertyLinksConnection>(
    this, "notused", SIGNAL(arraySelectionChanged()), proxy, prop);

  this->updateColorTransferFunction();
}

//-----------------------------------------------------------------------------
QPair<int, QString> pqDisplayColorWidget::arraySelection() const
{
  if (this->Variables->currentIndex() != -1)
    {
    return PropertyLinksConnection::convert(this->Variables->itemData(
        this->Variables->currentIndex()));
    }

  return QPair<int, QString>();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setArraySelection(
  const QPair<int, QString> & value)
{
  int association = value.first;
  const QString& arrayName = value.second;

  if (association < vtkDataObject::POINT || association > vtkDataObject::CELL)
    {
    qCritical("Only cell/point data coloring is currently supported by this widget.");
    association = vtkDataObject::POINT;
    }

  QVariant idata = this->itemData(association, arrayName);
  QIcon* icon = this->itemIcon(association, arrayName);

  int index = this->Variables->findData(idata);
  if (index == -1)
    {
    bool prev = this->Variables->blockSignals(true);
    this->Variables->addItem(*icon, arrayName, idata);
    this->Variables->blockSignals(prev);

    index = this->Variables->findData(idata);
    qDebug() << "(" << association << ", " << arrayName << " ) "
      "is not an array shown in the pqDisplayColorWidget currently. "
      "Will add a new entry for it";
    }
  Q_ASSERT(index != -1);
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
  return association == vtkDataObject::CELL?  this->CellDataIcon: this->PointDataIcon;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::refreshColorArrayNames()
{
  // Simply update the this->Variables combo-box with values from the domain.
  // Try to preserve the current text if possible.
  QVariant current = PropertyLinksConnection::convert(this->arraySelection());

  bool prev = this->Variables->blockSignals(true);

  this->Variables->clear();

  // add solid color.
  this->Variables->addItem(*this->SolidColorIcon, "Solid Color", QVariant());

  vtkSMArrayListDomain* domain = this->Internals->Domain;
  Q_ASSERT (domain);

  for (unsigned int cc=0, max = domain->GetNumberOfStrings(); cc < max; cc++)
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
    this->Variables->addItem(*icon, label, idata);
    }
  // doing this here, instead of in the construtor ensures that the
  // popup menu shows resonably on OsX.
  this->Variables->setMaxVisibleItems(
    this->Variables->count()+1);
  int idx = this->Variables->findData(current);
  if (idx >= 0)
    {
    this->Variables->setCurrentIndex(idx);
    }
  this->Variables->blockSignals(prev);
  this->Variables->setEnabled(this->Variables->count() > 0);
}

//-----------------------------------------------------------------------------
int pqDisplayColorWidget::componentNumber() const
{
  // here, we treat -1 as the magnitude (to be consistent with
  // VTK/vtkScalarsToColors). This mismatches with how
  // vtkPVArrayInformation refers to magnitude.
  int index = this->Components->currentIndex();
  return index >=0? this->Components->itemData(index).toInt() : -1;
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
    Q_ASSERT(index != -1);
    this->Components->setCurrentIndex(index);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::componentNumberChanged()
{
  if (this->ColorTransferFunction)
    {
    BEGIN_UNDO_SET("Change color component");
    int number = this->componentNumber();
    this->ColorTransferFunction->setVectorMode(
      number<0? pqScalarsToColors::MAGNITUDE : pqScalarsToColors::COMPONENT,
      number<0? 0 : number);

    // we could now respect some application setting to determine if the LUT is
    // to be reset.
    vtkSMProxy* reprProxy = this->Representation?
      this->Representation->getProxy() : NULL;
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(reprProxy);

    // Update scalar bars.
    vtkSMTransferFunctionProxy::UpdateScalarBarsComponentTitle(
      this->ColorTransferFunction->getProxy(),
      vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(reprProxy));
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
  const QString &arrayName = val.second;

  vtkPVDataInformation* dataInfo = this->Representation->getInputDataInformation();
  vtkPVArrayInformation* arrayInfo = dataInfo?
    dataInfo->GetArrayInformation(arrayName.toLatin1().data(), association) : NULL;
  if (!arrayInfo)
    {
    vtkPVDataInformation* reprInfo = this->Representation->getRepresentedDataInformation();
    arrayInfo = reprInfo?
      reprInfo->GetArrayInformation(arrayName.toLatin1().data(), association) : NULL;
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
  for (int cc=0, max=arrayInfo->GetNumberOfComponents(); cc < max; cc++)
    {
    if (cc==0 && max > 1)
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
      lut->getVectorMode() == pqScalarsToColors::MAGNITUDE? -1 :
      lut->getVectorComponent());
    this->connect(lut, SIGNAL(componentOrModeChanged()),
      SLOT(updateColorTransferFunction()), Qt::UniqueConnection);
    }
}
