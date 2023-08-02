// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPluginTreeWidgetEventTranslator_h
#define pqPluginTreeWidgetEventTranslator_h

#include "pqComponentsModule.h"
#include "pqWidgetEventTranslator.h"
#include <QPointer>

class QModelIndex;
class pqPluginTreeWidget;

class PQCOMPONENTS_EXPORT pqPluginTreeWidgetEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqPluginTreeWidgetEventTranslator(QObject* parentObject = nullptr);
  ~pqPluginTreeWidgetEventTranslator() override;

  using Superclass::translateEvent;
  bool translateEvent(QObject* Object, QEvent* Event, bool& Error) override;

private Q_SLOTS:
  void onItemChanged(const QModelIndex&);
  void onExpanded(const QModelIndex&);
  void onCollapsed(const QModelIndex&);
  void onCurrentChanged(const QModelIndex&);

private: // NOLINT(readability-redundant-access-specifiers)
  QString getIndexAsString(const QModelIndex&);

  pqPluginTreeWidgetEventTranslator(const pqPluginTreeWidgetEventTranslator&);
  pqPluginTreeWidgetEventTranslator& operator=(const pqPluginTreeWidgetEventTranslator&);

  QPointer<pqPluginTreeWidget> TreeView;
};

#endif // !pqPluginTreeWidgetEventTranslator_h
