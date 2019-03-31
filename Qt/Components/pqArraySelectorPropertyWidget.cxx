/*=========================================================================

   Program: ParaView
   Module:  pqArraySelectorPropertyWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqArraySelectorPropertyWidget.h"

#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMVectorProperty.h"

#include <QComboBox>
#include <QIcon>
#include <QPair>
#include <QPointer>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <cassert>

namespace
{
QIcon get_icon(int assoc)
{
  switch (assoc)
  {
    case vtkDataObject::POINT:
      return QIcon(":/pqWidgets/Icons/pqPointData16.png");
    case vtkDataObject::CELL:
      return QIcon(":/pqWidgets/Icons/pqCellData16.png");
    case vtkDataObject::FIELD:
      return QIcon(":/pqWidgets/Icons/pqGlobalData16.png");
    case vtkDataObject::VERTEX:
      return QIcon(":/pqWidgets/Icons/pqPointData16.png");
    case vtkDataObject::EDGE:
      return QIcon(":/pqWidgets/Icons/pqEdgeCenterData16.png");
    case vtkDataObject::ROW:
      return QIcon(":/pqWidgets/Icons/pqSpreadsheet16.png");
    default:
      return QIcon();
  }
}

QIcon get_none_icon()
{
  return QIcon(":/pqWidgets/Icons/pqCancel32.png");
}

QString get_label(const QString& name, bool is_partial)
{
  return is_partial ? name + " (partial)" : name;
}
}

class pqArraySelectorPropertyWidget::pqInternals
{
public:
  const int ArrayAssociationRole = Qt::UserRole + 1;
  const int ArrayNameRole = Qt::UserRole + 2;
  const int UnknownItemRole = Qt::UserRole + 3;
  const int AssociationUnspecified = -1;

  QPair<int, QString> Array = qMakePair(AssociationUnspecified, QString());
  QPointer<QComboBox> ComboBox;
  vtkWeakPointer<vtkSMArrayListDomain> Domain;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

  /**
   * Updates the ComboBox with values from the vtkSMArrayListDomain.
   * Also ensures that the ComboBox will always have an appropriately flagged entry
   * for current value (Array) if it is valid.
   */
  void updateDomain()
  {
    assert(this->ComboBox && this->Domain);
    const QSignalBlocker blocker(this->ComboBox);

    auto combobox = this->ComboBox;
    auto ald = this->Domain;

    combobox->clear();
    const auto none_string = ald->GetNoneString();
    for (unsigned int cc = 0, max = ald->GetNumberOfStrings(); cc < max; ++cc)
    {
      const auto aname = ald->GetString(cc);
      const bool is_partial = ald->IsArrayPartial(cc) ? true : false;
      const int assoc = ald->GetDomainAssociation(cc);
      const bool is_nonestring = (none_string && strcmp(none_string, aname) == 0);
      const int index = combobox->count();
      combobox->addItem(
        is_nonestring ? get_none_icon() : get_icon(assoc), ::get_label(aname, is_partial));
      combobox->setItemData(index, assoc, ArrayAssociationRole);
      combobox->setItemData(index, aname, ArrayNameRole);
    }

    int index = this->findData(this->Array.first, this->Array.second);
    if (index == -1)
    {
      index = this->addUnknownItem(this->Array.first, this->Array.second);
    }
    combobox->setCurrentIndex(index);
  }

  int addUnknownItem(int assoc, const QString& aname)
  {
    if (assoc <= 0 && aname.isEmpty())
    {
      return -1;
    }
    auto combobox = this->ComboBox;
    const int index = combobox->count();
    combobox->addItem(get_icon(assoc), ::get_label(aname, false) + " (?)");
    combobox->setItemData(index, QVariant(assoc), ArrayAssociationRole);
    combobox->setItemData(index, QVariant(aname), ArrayNameRole);
    combobox->setItemData(index, QVariant(true), UnknownItemRole);
    return index;
  }

  int findData(int assoc, const QString& aname)
  {
    auto combobox = this->ComboBox;
    if (assoc == AssociationUnspecified)
    {
      return combobox->findData(aname, ArrayNameRole);
    }
    else
    {
      for (int cc = 0, max = combobox->count(); cc < max; ++cc)
      {
        if (combobox->itemData(cc, ArrayAssociationRole).toInt() == assoc &&
          combobox->itemData(cc, ArrayNameRole).toString() == aname)
        {
          return cc;
        }
      }
    }
    return -1;
  }

  void pruneUnusedUnknownItems()
  {
    auto combobox = this->ComboBox;
    const int chosen_index = combobox->currentIndex();
    for (int cc = 0, max = combobox->count(); cc < max; ++cc)
    {
      if (cc != chosen_index && combobox->itemData(cc, UnknownItemRole).toBool())
      {
        combobox->removeItem(cc);
      }
    }
  }

  void updateArrayToCurrent()
  {
    auto combobox = this->ComboBox;
    const int chosen_index = combobox->currentIndex();
    this->Array.first = combobox->itemData(chosen_index, ArrayAssociationRole).toInt();
    this->Array.second = combobox->itemData(chosen_index, ArrayNameRole).toString();
  }
};

class pqArraySelectorPropertyWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
  using Superclass = pqPropertyLinksConnection;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = 0)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }

protected:
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    QList<QVariant> list = value.value<QList<QVariant> >();
    assert(list.size() == 2);

    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);
    helper.SetInputArrayToProcess(list[0].toInt(), list[1].toString().toUtf8().data());
  }

  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);

    QList<QVariant> list{ QVariant(helper.GetInputArrayAssociation()),
      QVariant(helper.GetInputArrayNameToProcess()) };
    return QVariant::fromValue(list);
  }

private:
  Q_DISABLE_COPY(PropertyLinksConnection);
};

//-----------------------------------------------------------------------------
pqArraySelectorPropertyWidget::pqArraySelectorPropertyWidget(
  vtkSMProperty* smproperty, vtkSMProxy* smproxy, QWidget* parentWdg)
  : Superclass(smproxy, parentWdg)
  , Internals(new pqArraySelectorPropertyWidget::pqInternals())
{
  this->setChangeAvailableAsChangeFinished(true);

  auto& internals = *this->Internals;

  auto l = new QVBoxLayout(this);
  l->setSpacing(0);
  l->setMargin(0);

  auto combobox = new QComboBox(this);
  combobox->setObjectName("ComboBox");
  l->addWidget(combobox);
  internals.ComboBox = combobox;

  auto ald = smproperty->FindDomain<vtkSMArrayListDomain>();
  assert(ald != nullptr);

  // update domain.
  internals.Domain = ald;
  internals.updateDomain();
  internals.VTKConnect->Connect(ald, vtkCommand::DomainModifiedEvent, this, SLOT(domainModified()));

  auto vproperty = vtkSMVectorProperty::SafeDownCast(smproperty);
  assert(vproperty);

  if (vproperty->GetNumberOfElements() == 2 || vproperty->GetNumberOfElements() == 5)
  {
    using ConnectionT = pqArraySelectorPropertyWidget::PropertyLinksConnection;
    this->links().addPropertyLink<ConnectionT>(
      this, "array", SIGNAL(arrayChanged()), smproxy, smproperty);
  }
  else if (vproperty->GetNumberOfElements() == 1)
  {
    this->links().addPropertyLink(this, "arrayName", SIGNAL(arrayChanged()), smproxy, smproperty);
  }

  QObject::connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int) {
    this->Internals->pruneUnusedUnknownItems();
    this->Internals->updateArrayToCurrent();
    emit this->arrayChanged();
  });
}

//-----------------------------------------------------------------------------
pqArraySelectorPropertyWidget::~pqArraySelectorPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqArraySelectorPropertyWidget::domainModified()
{
  auto& internals = *this->Internals;
  internals.updateDomain();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqArraySelectorPropertyWidget::array() const
{
  QList<QVariant> val{ this->arrayAssociation(), this->arrayName() };
  return val;
}

//-----------------------------------------------------------------------------
void pqArraySelectorPropertyWidget::setArray(const QList<QVariant>& val)
{
  assert(val.size() == 2);
  this->setArray(val[0].toInt(), val[1].toString());
}

//-----------------------------------------------------------------------------
QString pqArraySelectorPropertyWidget::arrayName() const
{
  return this->Internals->Array.second;
}

//-----------------------------------------------------------------------------
int pqArraySelectorPropertyWidget::arrayAssociation() const
{
  return this->Internals->Array.first;
}

//-----------------------------------------------------------------------------
void pqArraySelectorPropertyWidget::setArray(int assoc, const QString& val)
{
  auto& internals = *this->Internals;
  internals.Array = qMakePair(assoc, val);
  int index = internals.findData(assoc, val);
  if (index == -1)
  {
    index = internals.addUnknownItem(assoc, val);
  }
  internals.ComboBox->setCurrentIndex(index);
  internals.pruneUnusedUnknownItems();
}

//-----------------------------------------------------------------------------
void pqArraySelectorPropertyWidget::setArrayName(const QString& name)
{
  this->setArray(-1, name);
}
