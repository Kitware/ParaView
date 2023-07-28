// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTreeWidgetItem_h
#define pqTreeWidgetItem_h

#include "pqWidgetsModule.h"
#include <QTreeWidgetItem>

/**
 * pqTreeWidgetItem is a QTreeWidgetItem with callbacks for whenever the data
 * for the pqTreeWidgetItem changes. This is designed as a replacement for
 * pqTreeWidgetItemObject avoid the need for pqTreeWidgetItem to be a QObject
 * subclass, thus keeping them light-weight.
 */
class PQWIDGETS_EXPORT pqTreeWidgetItem : public QTreeWidgetItem
{
  typedef QTreeWidgetItem Superclass;

public:
  pqTreeWidgetItem(int atype = UserType)
    : Superclass(atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(const QStringList& strings, int atype = UserType)
    : Superclass(strings, atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(QTreeWidget* aparent, int atype = UserType)
    : Superclass(aparent, atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(QTreeWidget* aparent, const QStringList& strings, int atype = UserType)
    : Superclass(aparent, strings, atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(QTreeWidget* aparent, QTreeWidgetItem* preceding, int atype = UserType)
    : Superclass(aparent, preceding, atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(QTreeWidgetItem* aparent, int atype = UserType)
    : Superclass(aparent, atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(QTreeWidgetItem* aparent, const QStringList& strings, int atype = UserType)
    : Superclass(aparent, strings, atype)
    , CallbackHandler(nullptr)
  {
  }
  pqTreeWidgetItem(QTreeWidgetItem* aparent, QTreeWidgetItem* preceding, int atype = UserType)
    : Superclass(aparent, preceding, atype)
    , CallbackHandler(nullptr)
  {
  }

  /**
   * overload setData() to call callbacks if set.
   */
  void setData(int column, int role, const QVariant& v) override;

  class pqCallbackHandler
  {
  public:
    virtual ~pqCallbackHandler() = default;

    /**
     * Called to indicate that the data is about to be changed.
     */
    virtual void dataAboutToChange(pqTreeWidgetItem* /*item*/, int /*column*/, int /*role*/){};

    /**
     * Called to indicate that the data is about to be changed.
     */
    virtual void checkStateAboutToChange(pqTreeWidgetItem* /*item*/, int /*column*/){};

    /**
     * Called to indicate that the check state for the item has been changed.
     */
    virtual void checkStateChanged(pqTreeWidgetItem* /*item*/, int /*column*/){};

    /**
     * Called to indicate that the data has been changed.
     */
    virtual void dataChanged(pqTreeWidgetItem* /*item*/, int /*column*/, int /*role*/){};

    /**
     * Called to check if the change has to be accepted or rejected.
     */
    virtual bool acceptChange(pqTreeWidgetItem* /*item*/, const QVariant& /*curValue*/,
      const QVariant& /*newValue*/, int /*column*/, int /*role*/)
    {
      return true;
    }
  };

  /**
   * Set the name of the callback slot to call
   */
  void setCallbackHandler(pqCallbackHandler* hdlr) { this->CallbackHandler = hdlr; }

protected:
  pqCallbackHandler* CallbackHandler;
};

#endif
