// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSettings.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDockWidget>
#include <QFile>
#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QDesktopWidget>
#endif

#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"

//-----------------------------------------------------------------------------
pqSettings::pqSettings(const QString& org, const QString& app, QObject* prnt)
  : Superclass(org, app, prnt)
{
}
//-----------------------------------------------------------------------------
pqSettings::pqSettings(Scope spe, const QString& org, const QString& app, QObject* prnt)
  : Superclass(spe, org, app, prnt)
{
}

//-----------------------------------------------------------------------------
pqSettings::pqSettings(Format fmt, Scope spe, const QString& org, const QString& app, QObject* prnt)
  : Superclass(fmt, spe, org, app, prnt)
{
}

//-----------------------------------------------------------------------------
pqSettings::pqSettings(const QString& fn, Format fmt, QObject* prnt)
  : Superclass(fn, fmt, prnt)
{
}

//-----------------------------------------------------------------------------
pqSettings::pqSettings(QObject* prnt)
  : Superclass(prnt)
{
}

//-----------------------------------------------------------------------------
pqSettings::~pqSettings() = default;

//-----------------------------------------------------------------------------
QString pqSettings::backup(const QString& argName)
{
  this->sync();

  QString fname = argName.isEmpty() ? (this->fileName() + ".bak") : argName;
  QFile::remove(fname);
  return QFile::copy(this->fileName(), fname) ? fname : QString();
}

//-----------------------------------------------------------------------------
void pqSettings::alertSettingsModified()
{
  Q_EMIT this->modified();
}

//-----------------------------------------------------------------------------
void pqSettings::saveState(const QDialog& dialog, const QString& key)
{
  this->beginGroup(key);
  this->setValue("Position", dialog.pos());
  this->setValue("Size", dialog.size());
  // let's add a PID to avoid restoring dialog position across different
  // sessions. This avoids issues reported in #18163.
  this->setValue("PID", QCoreApplication::applicationPid());
  this->endGroup();
}

//-----------------------------------------------------------------------------
void pqSettings::restoreState(const QString& key, QDialog& dialog)
{
  this->beginGroup(key);

  if (this->contains("Size"))
  {
    dialog.resize(this->value("Size").toSize());
  }

  // restore position only if it is the same process.
  if (this->value("PID").value<qint64>() == QCoreApplication::applicationPid() &&
    this->contains("Position"))
  {
    dialog.move(this->value("Position").toPoint());
  }
  this->endGroup();
}

//-----------------------------------------------------------------------------
void pqSettings::saveState(const QMainWindow& window, const QString& key)
{
  this->beginGroup(key);
  this->setValue("Size", window.size());
  this->setValue("Layout", window.saveState());
  this->endGroup();
}

//-----------------------------------------------------------------------------
void pqSettings::restoreState(const QString& key, QMainWindow& window)
{
  this->beginGroup(key);

  if (this->contains("Size"))
  {
    window.resize(this->value("Size").toSize());
  }

  if (this->contains("Layout"))
  {
    window.restoreState(this->value("Layout").toByteArray());

    QList<QDockWidget*> dockWidgets = window.findChildren<QDockWidget*>();
    Q_FOREACH (QDockWidget* dock_widget, dockWidgets)
    {
      if (dock_widget->isFloating() == true)
      {
        sanityCheckDock(dock_widget);
      }
    }
  }

  this->endGroup();
}

//-----------------------------------------------------------------------------
void pqSettings::saveInQSettings(const char* key, vtkSMProperty* smproperty)
{
  // FIXME: handle all property types. This will only work for single value
  // properties.
  if (smproperty->IsA("vtkSMIntVectorProperty") || smproperty->IsA("vtkSMIdTypeVectorProperty"))
  {
    this->setValue(key, vtkSMPropertyHelper(smproperty).GetAsInt());
  }
  else if (smproperty->IsA("vtkSMDoubleVectorProperty"))
  {
    this->setValue(key, vtkSMPropertyHelper(smproperty).GetAsDouble());
  }
  else if (smproperty->IsA("vtkSMStringVectorProperty"))
  {
    this->setValue(key, vtkSMPropertyHelper(smproperty).GetAsString());
  }
}

//-----------------------------------------------------------------------------
void pqSettings::sanityCheckDock(QDockWidget* dock_widget)
{
  if (nullptr == dock_widget)
  {
    return;
  }

  QPoint dockTopLeft = dock_widget->pos();
  QRect dockRect(dockTopLeft, dock_widget->size());

  QRect geometry = QRect(dockTopLeft, dock_widget->frameSize());
  int titleBarHeight = geometry.height() - dockRect.height();

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
  QDesktopWidget desktop;
  QRect screenRect = desktop.availableGeometry(dock_widget);
#else
  auto desktop = dock_widget->screen();
  QRect screenRect = desktop->availableGeometry();
#endif

  QRect desktopRect = QGuiApplication::primaryScreen()
                        ->availableGeometry(); // Should give us the entire Desktop geometry
  // Ensure the top left corner of the window is on the screen
  if (!screenRect.contains(dockTopLeft))
  {
    // Are we High?
    if (dockTopLeft.y() < screenRect.y())
    {
      dock_widget->move(dockRect.x(), screenRect.y());
      dockTopLeft = dock_widget->pos();
      dockRect = QRect(dockTopLeft, dock_widget->frameSize());
    }
    // Are we low
    if (dockTopLeft.y() > screenRect.y() + screenRect.height())
    {
      dock_widget->move(dockRect.x(), screenRect.y() + screenRect.height() - 20);
      dockTopLeft = dock_widget->pos();
      dockRect = QRect(dockTopLeft, dock_widget->frameSize());
    }
    // Are we left
    if (dockTopLeft.x() < screenRect.x())
    {
      dock_widget->move(screenRect.x(), dockRect.y());
      dockTopLeft = dock_widget->pos();
      dockRect = QRect(dockTopLeft, dock_widget->frameSize());
    }
    // Are we right
    if (dockTopLeft.x() > screenRect.x() + screenRect.width())
    {
      dock_widget->move(screenRect.x() + screenRect.width() - dockRect.width(), dockRect.y());
      dockTopLeft = dock_widget->pos();
      dockRect = QRect(dockTopLeft, dock_widget->frameSize());
    }

    dockTopLeft = dock_widget->pos();
    dockRect = QRect(dockTopLeft, dock_widget->frameSize());
  }

  if (!desktopRect.contains(dockRect))
  {
    // Are we too wide
    if (dockRect.x() + dockRect.width() > screenRect.x() + screenRect.width())
    {
      if (screenRect.x() + screenRect.width() - dockRect.width() > screenRect.x())
      {
        // Move dock side to side
        dockRect.setX(screenRect.x() + screenRect.width() - dockRect.width());
        dock_widget->move(dockRect.x(), dockRect.y());
        dockTopLeft = dock_widget->pos();
        dockRect = QRect(dockTopLeft, dock_widget->frameSize());
      }
      else
      {
        // Move dock side to side + resize to fit
        dockRect.setX(screenRect.x() + screenRect.width() - dockRect.width());
        dockRect.setWidth(screenRect.width());
        dock_widget->resize(dockRect.width(), dockRect.height());
        dock_widget->move(dockRect.x(), dockRect.y());
        dockTopLeft = dock_widget->pos();
        dockRect = QRect(dockTopLeft, dock_widget->frameSize());
      }
    }

    dockTopLeft = dock_widget->pos();
    dockRect = QRect(dockTopLeft, dock_widget->frameSize());
    // Are we too Tall
    if (dockRect.y() + dockRect.height() > screenRect.y() + screenRect.height())
    {
      // See if we can move it more on screen so that the entire dock is on screen
      if (screenRect.y() + screenRect.height() - dockRect.height() > screenRect.y())
      {
        // Move dock up
        dockRect.setY(screenRect.y() + screenRect.height() - dockRect.height());
        dock_widget->move(dockRect.x(), dockRect.y());
        dockTopLeft = dock_widget->pos();
        dockRect = QRect(dockTopLeft, dock_widget->frameSize());
      }
      else
      {
        // Move dock up + resize to fit
        dock_widget->resize(dockRect.width(), screenRect.height() - titleBarHeight);
        dock_widget->move(dockRect.x(), screenRect.y());
        dockTopLeft = dock_widget->pos();
        dockRect = QRect(dockTopLeft, dock_widget->frameSize());
      }
    }
  }
}
