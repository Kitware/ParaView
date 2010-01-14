/*=========================================================================

   Program: ParaView
   Module:    pqPythonToolsWidget.h

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
#ifndef _pqPythonToolsWidget_h
#define _pqPythonToolsWidget_h

#include "QtPythonExport.h"
#include <QWidget>

class QListWidgetItem;
class QModelIndex;
class pqServer;
class pqPythonDialog;

/// Note, this class uses python from its constructor, so it should be
/// instantiated after python is initialized or it will cause python
/// to become initialized (by calling pqPythonManager::pythonShellDialog())
class QTPYTHON_EXPORT pqPythonToolsWidget : public QWidget
{
  Q_OBJECT
public:
  typedef QWidget Superclass;
  pqPythonToolsWidget(QWidget* p);
  ~pqPythonToolsWidget();

  bool contains(const QString& filePath);
  void setScriptDirectory(const QString& dir);
  QString scriptDirectory();

protected slots:

  // Directory view tab
  void onScriptDirectoryEditFinished();
  void onChooseDirectoryClicked();
  void onRefreshClicked();
  void onRunSelectedClicked();
  void onNewScriptClicked();
  void onAddToMacrosClicked();
  void selectionChanged(const QModelIndex&);
  void itemActivated(const QModelIndex&);

  // Trace tab
  void onStartTraceClicked();
  void onStopTraceClicked();
  void onTraceStateClicked();
  void onShowTraceClicked();
  void onSaveTraceClicked();
  void onEditTraceClicked();
  void onInterpreterReset();

  // Macros tab
  void addMacroToListBox(const QString& macroName, const QString& filename);
  void onRemoveMacroClicked();
  void onShowMenuChecked();
  void onMacroListSelectionChanged();
  void onMacroNameChanged(QListWidgetItem* item);

signals:
  void addMacroRequested(const QString& macroName, const QString& filename);
  void removeMacroRequested(const QString& filename);

protected:

  QString getTraceString();
  QString getPVModuleDirectory();
  pqPythonDialog* pythonShellDialog();

  // Description:
  // Clears the macro listbox and repopulates it from the macros stored
  // in pqSettings.
  void resetMacroList();

private:
  class pqInternal;
  pqInternal* Internal;

};

#endif //ifndef _pqPythonToolsWidget_h

