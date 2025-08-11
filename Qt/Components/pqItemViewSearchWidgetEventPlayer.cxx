// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqItemViewSearchWidgetEventPlayer.h"
#include "pqItemViewSearchWidget.h"

#include <QAbstractItemView>
#include <QPointer>

//----------------------------------------------------------------------------
pqItemViewSearchWidgetEventPlayer::pqItemViewSearchWidgetEventPlayer(QObject* p)
  : Superclass(p)
{
}

//----------------------------------------------------------------------------
pqItemViewSearchWidgetEventPlayer::~pqItemViewSearchWidgetEventPlayer() = default;

//----------------------------------------------------------------------------
bool pqItemViewSearchWidgetEventPlayer::playEvent(
  QObject* w, const QString& command, const QString& arguments, bool& error)
{
  Q_UNUSED(error);
  if (command == pqItemViewSearchWidgetEventPlayer::EVENT_NAME())
  {
    if (arguments == "ctrlF")
    {
      QPointer<QAbstractItemView> focusItemView = qobject_cast<QAbstractItemView*>(w);
      if (!focusItemView)
      {
        return false;
      }
      QPointer<pqItemViewSearchWidget> searchWidget = new pqItemViewSearchWidget(focusItemView);
      QObject::connect(searchWidget, &QWidget::close, searchWidget, &QObject::deleteLater);
      searchWidget->showSearchWidget();
    }
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
const QString& pqItemViewSearchWidgetEventPlayer::EVENT_NAME()
{
  static const QString eventName("launchSearchWidget");
  return eventName;
}
