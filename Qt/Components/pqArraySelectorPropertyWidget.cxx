// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
      return QIcon(":/pqWidgets/Icons/pqPointData.svg");
    case vtkDataObject::CELL:
      return QIcon(":/pqWidgets/Icons/pqCellData.svg");
    case vtkDataObject::FIELD:
      return QIcon(":/pqWidgets/Icons/pqGlobalData.svg");
    case vtkDataObject::VERTEX:
      return QIcon(":/pqWidgets/Icons/pqPointData.svg");
    case vtkDataObject::EDGE:
      return QIcon(":/pqWidgets/Icons/pqEdgeCenterData.svg");
    case vtkDataObject::ROW:
      return QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg");
    default:
      return QIcon();
  }
}

QIcon get_none_icon()
{
  return QIcon(":/pqWidgets/Icons/pqCancel.svg");
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
  std::vector<QPair<int, QString>> KnownArrays;
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

    for (const auto& array : this->KnownArrays)
    {
      const int& assoc = array.first;
      const auto& aname = array.second;
      this->addItem(assoc, aname, false);
    }

    for (unsigned int cc = 0, max = ald->GetNumberOfStrings(); cc < max; ++cc)
    {
      const auto aname = ald->GetString(cc);
      const bool is_partial = ald->IsArrayPartial(cc) ? true : false;
      const int icon_association = ald->GetDomainAssociation(cc);
      const int association = ald->GetFieldAssociation(cc);
      const bool is_nonestring = (none_string && strcmp(none_string, aname) == 0);
      const int index = combobox->count();
      combobox->addItem(is_nonestring ? get_none_icon() : get_icon(icon_association),
        ::get_label(aname, is_partial));
      combobox->setItemData(index, association, ArrayAssociationRole);
      combobox->setItemData(index, aname, ArrayNameRole);
    }

    int index = this->findData(this->Array.first, this->Array.second);
    if (index == -1)
    {
      index = this->addItem(this->Array.first, this->Array.second, true);
    }
    combobox->setCurrentIndex(index);
  }

  int addItem(int assoc, const QString& aname, bool unknown)
  {
    if (assoc <= 0 && aname.isEmpty())
    {
      return -1;
    }
    auto combobox = this->ComboBox;
    const int index = combobox->count();
    combobox->addItem(get_icon(assoc), ::get_label(aname, false) + (unknown ? " (?)" : ""));
    combobox->setItemData(index, QVariant(assoc), ArrayAssociationRole);
    combobox->setItemData(index, QVariant(aname), ArrayNameRole);
    combobox->setItemData(index, QVariant(unknown), UnknownItemRole);
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
    QObject* parentObject = nullptr)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }

protected:
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    QList<QVariant> list = value.value<QList<QVariant>>();
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
  : pqArraySelectorPropertyWidget(smproperty, smproxy, {}, parentWdg)
{
}

//-----------------------------------------------------------------------------
pqArraySelectorPropertyWidget::pqArraySelectorPropertyWidget(vtkSMProperty* smproperty,
  vtkSMProxy* smproxy, std::initializer_list<QPair<int, QString>> KnownArrays, QWidget* parentWdg)
  : Superclass(smproxy, parentWdg)
  , Internals(new pqArraySelectorPropertyWidget::pqInternals())
{
  this->setChangeAvailableAsChangeFinished(true);

  auto& internals = *this->Internals;
  internals.KnownArrays = KnownArrays;

  auto l = new QVBoxLayout(this);
  l->setSpacing(0);
  l->setContentsMargins(0, 0, 0, 0);

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

  QObject::connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this](int)
    {
      this->Internals->pruneUnusedUnknownItems();
      this->Internals->updateArrayToCurrent();
      Q_EMIT this->arrayChanged();
    });
}

//-----------------------------------------------------------------------------
pqArraySelectorPropertyWidget::~pqArraySelectorPropertyWidget() = default;

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
    index = internals.addItem(assoc, val, /*unknown=*/true);
  }
  internals.ComboBox->setCurrentIndex(index);
  internals.pruneUnusedUnknownItems();
}

//-----------------------------------------------------------------------------
void pqArraySelectorPropertyWidget::setArrayName(const QString& name)
{
  this->setArray(-1, name);
}
