/*=========================================================================

   Program: ParaView
   Module:    pqPythonScriptEditor.h

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
#ifndef _pqPythonScriptEditor_h
#define _pqPythonScriptEditor_h

#include "pqPythonModule.h"
#include <QMainWindow>

class QAction;
class QMenu;
class QTextEdit;

class pqPythonManager;
class pqPythonSyntaxHighlighter;

class PQPYTHON_EXPORT pqPythonScriptEditor : public QMainWindow
{
  Q_OBJECT

public:
  typedef QMainWindow Superclass;
  pqPythonScriptEditor(QWidget* parent = 0);

  void setSaveDialogDefaultDirectory(const QString& dir);
  void setPythonManager(pqPythonManager* manager);

  /*
   * Scroll the editor to the bottom of the scroll area
   */
  void scrollToBottom();

public Q_SLOTS:

  void open(const QString& filename);
  void setText(const QString& text);

  // Returns true if the new file was created.  If the user was prompted to save
  // data before creating a new file and clicked cancel, then no new file will
  // be created and this method returns false.
  bool newFile();

Q_SIGNALS:
  void fileSaved();

protected:
  void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
  void open();
  bool save();
  bool saveAs();
  bool saveAsMacro();
  void documentWasModified();

private:
  void createActions();
  void createMenus();
  void createStatusBar();
  void readSettings();
  void writeSettings();
  bool maybeSave();
  void loadFile(const QString& fileName);
  bool saveFile(const QString& fileName);
  void setCurrentFile(const QString& fileName);
  QString strippedName(const QString& fullFileName);

  QTextEdit* TextEdit;
  QString CurrentFile;
  QString DefaultSaveDirectory;

  QMenu* fileMenu;
  QMenu* editMenu;
  QMenu* helpMenu;
  QAction* newAct;
  QAction* openAct;
  QAction* saveAct;
  QAction* saveAsAct;
  QAction* saveAsMacroAct;
  QAction* exitAct;
  QAction* cutAct;
  QAction* copyAct;
  QAction* pasteAct;

  pqPythonManager* pythonManager;

  pqPythonSyntaxHighlighter* SyntaxHighlighter;
};

#endif
