/*=========================================================================

   Program: ParaView
   Module:    pqPythonMacroSupervisor.h

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
#ifndef _pqPythonMacroSupervisor_h
#define _pqPythonMacroSupervisor_h

#include "QtPythonExport.h"
#include <QObject>
#include <QMap>

class QAction;

class QTPYTHON_EXPORT pqPythonMacroSupervisor : public QObject
{
  Q_OBJECT
public:
  pqPythonMacroSupervisor(QObject* p=0);
  ~pqPythonMacroSupervisor();

  // Description:
  // Add a widget to be given macro actions.  QActions representing script macros
  // will be added to the widget.  This could be a QToolBar, QMenu, or other type
  // of widget.
  void addWidgetForMacros(QWidget* widget);

  // Description:
  // Lookup and return a macro action by filename.
  // If it does not exist, return null.
  QAction* getMacro(const QString& fileName);

  // Description:
  // Looks in pqSettings to get the stored macros.  In the returned map,
  // the keys are filenames and values are macro names.
  static QMap<QString, QString> getStoredMacros();

  // Description:
  // Stores a macro with the given name and filename in pqSettings.
  // If a macro with the given filename already exists, its name will
  // be updated.
  static void storeMacro(const QString& macroName, const QString& filename);

  // Description:
  // Removes a macro with the given filename from pqSettings, if it exists.
  static void removeStoredMacro(const QString& filename);

signals:

  // Description:
  // Emitted when a macro has been triggered.
  void executeScriptRequested(const QString& filename);

public slots:

  // Description:
  // Add an action with the given name and filename.  If there is already
  // a macro with the given filename it's macroname will be updated to the
  // one given.  Macro names do not have to be unique.  Note, this does not
  // store the macro in pqSettings, you must still call storeMacro yourself.
  void addMacro(const QString& macroName, const QString& filename);

  // Description:
  // Remove an action with the given filename.  Note, this does not remove
  // the macro from pqSettings, you must call removeStoredMacro yourself.
  void removeMacro(const QString& filename);

protected slots:

  // Description:
  // If the sender is a QAction managed by this class, the filename will be
  // looked up and the signal requestExecuteScript will be emitted.
  void onMacroTriggered();

protected:

  // Description:
  // Removes all actions and re-adds actions for each macro stored in pqSettings.
  void resetActions();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif //ifndef _pqPythonMacroSupervisor_h

