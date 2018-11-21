/*=========================================================================

   Program: ParaView
   Module:    pqSettings.h

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
#ifndef _pqSettings_h
#define _pqSettings_h

#include "pqCoreModule.h"
#include <QSettings>

class QDialog;
class QMainWindow;
class QDockWidget;
class vtkSMProperty;

/**
* pqSettings extends QSettings to add support for following:
* \li saving/restoring window/dialog geometry.
*/
class PQCORE_EXPORT pqSettings : public QSettings
{
  Q_OBJECT
  typedef QSettings Superclass;

public:
  pqSettings(
    const QString& organization, const QString& application = QString(), QObject* parent = nullptr);
  pqSettings(Scope scope, const QString& organization, const QString& application = QString(),
    QObject* parent = nullptr);
  pqSettings(Format format, Scope scope, const QString& organization,
    const QString& application = QString(), QObject* parent = nullptr);
  pqSettings(const QString& fileName, Format format, QObject* parent = nullptr);
  pqSettings(QObject* parent = nullptr);
  ~pqSettings() override;

  void saveState(const QMainWindow& window, const QString& key);
  void saveState(const QDialog& dialog, const QString& key);

  void restoreState(const QString& key, QMainWindow& window);
  void restoreState(const QString& key, QDialog& dialog);

  /**
  * Calling this method will cause the modified signal to be emitted.
  */
  void alertSettingsModified();

  /**
  * Save a property value to a given setting name
  */
  void saveInQSettings(const char* key, vtkSMProperty* smproperty);

  /**
   * Creates a new backup file for the current settings.
   * If `filename` is empty, then a backup file name will automatically be
   * picked. On success returns the backup file name, on failure an empty string
   * is returned.
   */
  QString backup(const QString& filename = QString());

private:
  /**
  * ensure that when window state is being loaded, if dock windows are
  * beyond the viewport, we correct them.
  */
  void sanityCheckDock(QDockWidget* dock_widget);
signals:
  void modified();
};

#endif
