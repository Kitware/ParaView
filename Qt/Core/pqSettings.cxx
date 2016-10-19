/*=========================================================================

   Program: ParaView
   Module:    pqSettings.cxx

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
#include <pqSettings.h>

#include <QDesktopWidget>
#include <QDialog>
#include <QDockWidget>
#include <QFile>
#include <QMainWindow>

#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"

namespace
{
class pqSettingsCleaner : public QObject
{
  QString Filename;

public:
  pqSettingsCleaner(const QString& filename, QObject* parentObject)
    : QObject(parentObject)
    , Filename(filename)
  {
  }

  virtual ~pqSettingsCleaner() { QFile::remove(this->Filename); }
};
}

//-----------------------------------------------------------------------------
pqSettings::pqSettings(const QString& organization, const QString& application, QObject* p)
  : QSettings(QSettings::IniFormat, QSettings::UserScope, organization, application, p)
{
}

//-----------------------------------------------------------------------------
pqSettings::pqSettings(const QString& filename, bool temporary, QObject* parentObject)
  : QSettings(filename, QSettings::IniFormat, parentObject)
{
  if (temporary)
  {
    new pqSettingsCleaner(filename, this);
  }
}

//-----------------------------------------------------------------------------
pqSettings::pqSettings(const QString& fname, Format fmt, QObject* parentObject)
  : Superclass(fname, fmt, parentObject)
{
}

//-----------------------------------------------------------------------------
void pqSettings::alertSettingsModified()
{
  emit this->modified();
}

//-----------------------------------------------------------------------------
void pqSettings::saveState(const QMainWindow& window, const QString& key)
{
  this->beginGroup(key);
  this->setValue("Position", window.pos());
  this->setValue("Size", window.size());
  this->setValue("Layout", window.saveState());
  this->endGroup();
}

//-----------------------------------------------------------------------------
void pqSettings::saveState(const QDialog& dialog, const QString& key)
{
  this->beginGroup(key);
  this->setValue("Position", dialog.pos());
  this->setValue("Size", dialog.size());
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

  if (this->contains("Position"))
  {
    QPoint windowTopLeft = this->value("Position").toPoint();
    QRect mwRect(windowTopLeft, window.size());

    QDesktopWidget desktop;
    QRect desktopRect = desktop.availableGeometry(desktop.primaryScreen());
    // try moving it to keep size
    if (!desktopRect.contains(mwRect))
    {
      mwRect = QRect(desktopRect.topLeft(), window.size());
    }
    // still doesn't fit, resize it
    if (!desktopRect.contains(mwRect))
    {
      mwRect = QRect(desktopRect.topLeft(), window.size());
      window.resize(desktopRect.size());
    }
    window.move(mwRect.topLeft());
  }

  if (this->contains("Layout"))
  {
    window.restoreState(this->value("Layout").toByteArray());

    QList<QDockWidget*> dockWidgets = window.findChildren<QDockWidget*>();
    foreach (QDockWidget* dock_widget, dockWidgets)
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
void pqSettings::restoreState(const QString& key, QDialog& dialog)
{
  this->beginGroup(key);

  if (this->contains("Size"))
  {
    dialog.resize(this->value("Size").toSize());
  }

  if (this->contains("Position"))
  {
    dialog.move(this->value("Position").toPoint());
  }

  this->endGroup();
}

//-----------------------------------------------------------------------------
void pqSettings::sanityCheckDock(QDockWidget* dock_widget)
{
  QDesktopWidget desktop;
  int screen = -1;
  if (NULL == dock_widget)
  {
    return;
  }

  QPoint dockTopLeft = dock_widget->pos();
  QRect dockRect(dockTopLeft, dock_widget->size());

  QRect geometry = QRect(dockTopLeft, dock_widget->frameSize());
  int titleBarHeight = geometry.height() - dockRect.height();

  screen = desktop.screenNumber(dock_widget);
  if (screen == -1) // Dock is at least partially on a screen
  {
    screen = desktop.screenNumber(dockTopLeft);
  }

  QRect screenRect = desktop.availableGeometry(screen);
  QRect desktopRect = desktop.availableGeometry(); // SHould give us the entire Desktop geometry
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
