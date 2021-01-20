/*=========================================================================

   Program: ParaView
   Module:    pqPythonScriptEditor.cxx

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
#include "pqPythonScriptEditor.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPythonManager.h"
#include "pqPythonSyntaxHighlighter.h"
#include "pqPythonTextArea.h"
#include "pqSettings.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextStream>

#include <vtkPythonInterpreter.h>

//-----------------------------------------------------------------------------
pqPythonScriptEditor::pqPythonScriptEditor(QWidget* p)
  : QMainWindow(p)
  , TextArea(new pqPythonTextArea(this))
  , pythonManager(nullptr)
{
  this->setCentralWidget(this->TextArea);

  this->createActions();
  this->createMenus();
  this->createStatusBar();
  this->resize(300, 450);
  pqApplicationCore::instance()->settings()->restoreState("PythonScriptEditor", *this);
  vtkPythonInterpreter::Initialize();

  this->setWindowTitle(tr("ParaView Python Script Editor"));

  const auto StrippedName = [](const QString& s) { return QFileInfo(s).fileName(); };

  connect(this->TextArea, &pqPythonTextArea::FileSavedAsMacro,
    [this, &StrippedName](const QString& filename) {
      if (pythonManager)
      {
        pythonManager->updateMacroList();
      }

      this->setWindowTitle(tr("%1[*] - %2").arg(StrippedName(filename)).arg(tr("Script Editor")));
      this->statusBar()->showMessage(tr("File %1 saved as macro").arg(filename), 4000);
    });

  connect(
    this->TextArea, &pqPythonTextArea::FileSaved, [this, &StrippedName](const QString& filename) {
      this->setWindowTitle(tr("%1[*] - %2").arg(StrippedName(filename)).arg(tr("Script Editor")));
      this->statusBar()->showMessage(tr("File %1 saved").arg(filename), 4000);
    });

  connect(
    this->TextArea, &pqPythonTextArea::FileOpened, [this, &StrippedName](const QString& filename) {
      this->setWindowTitle(tr("%1[*] - %2").arg(StrippedName(filename)).arg(tr("Script Editor")));
      this->statusBar()->showMessage(tr("File %1 opened").arg(filename), 4000);
    });
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::closeEvent(QCloseEvent* e)
{
  if (this->TextArea->SaveOnClose())
  {
    pqApplicationCore::instance()->settings()->saveState(*this, "PythonScriptEditor");
    e->accept();
  }
  else
  {
    e->ignore();
  }
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::open(const QString& filename)
{
  this->TextArea->OpenFile(filename);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setSaveDialogDefaultDirectory(const QString& dir)
{
  this->TextArea->SetDefaultSaveDirectory(dir);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setPythonManager(pqPythonManager* manager)
{
  this->pythonManager = manager;
  this->TextArea->GetSaveFileAsMacroAction()->setEnabled(manager != nullptr);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createActions()
{
  QAction* newFileAction = this->TextArea->GetNewFileAction();
  newFileAction->setText(tr("&New"));
  newFileAction->setShortcut(tr("CtrL+N"));
  newFileAction->setStatusTip(tr("Create a new file"));

  QAction* openFileAction = this->TextArea->GetOpenFileAction();
  openFileAction->setText(tr("&Open..."));
  openFileAction->setShortcut(tr("CtrL+O"));
  openFileAction->setStatusTip(tr("Open an existing file"));

  QAction* saveFileAction = this->TextArea->GetSaveFileAction();
  saveFileAction->setText(tr("&Save"));
  saveFileAction->setShortcut(tr("CtrL+S"));
  saveFileAction->setStatusTip(tr("Save the document to disk"));

  QAction* saveFileAsAction = this->TextArea->GetSaveFileAsAction();
  saveFileAsAction->setText(tr("Save &As..."));
  saveFileAsAction->setStatusTip(tr("Save the document under a new name"));

  QAction* saveFileAsMacroAction = this->TextArea->GetSaveFileAsMacroAction();
  saveFileAsMacroAction->setText(tr("Save As &Macro..."));
  saveFileAsMacroAction->setStatusTip(tr("Save the document as a Macro"));
  saveFileAsMacroAction->setEnabled(false);

  this->exitAct = new QAction(tr("C&lose"), this);
  this->exitAct->setShortcut(tr("Ctrl+W"));
  this->exitAct->setStatusTip(tr("Close the script editor"));
  this->connect(this->exitAct, SIGNAL(triggered()), this, SLOT(close()));

  this->cutAct = new QAction(tr("Cu&t"), this);
  this->cutAct->setShortcut(tr("Ctrl+X"));
  this->cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                                "clipboard"));
  this->connect(this->cutAct, SIGNAL(triggered()), this->TextArea->GetTextEdit(), SLOT(cut()));

  this->copyAct = new QAction(tr("&Copy"), this);
  this->copyAct->setShortcut(tr("Ctrl+C"));
  this->copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                                 "clipboard"));
  this->connect(this->copyAct, SIGNAL(triggered()), this->TextArea->GetTextEdit(), SLOT(copy()));

  this->pasteAct = new QAction(tr("&Paste"), this);
  this->pasteAct->setShortcut(tr("Ctrl+V"));
  this->pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                                  "selection"));
  this->connect(this->pasteAct, SIGNAL(triggered()), this->TextArea->GetTextEdit(), SLOT(paste()));

  QAction* undoAct = this->TextArea->GetUndoAction();
  undoAct->setShortcut(tr("Ctrl+Z"));
  undoAct->setStatusTip(tr("Undo the last edit of the file"));

  QAction* redoAct = this->TextArea->GetRedoAction();
  redoAct->setShortcut(tr("Ctrl+Y"));
  redoAct->setStatusTip(tr("Redo the last undo of the file"));

  this->cutAct->setEnabled(false);
  this->copyAct->setEnabled(false);

  this->connect(this->TextArea->GetTextEdit(), SIGNAL(copyAvailable(bool)), this->cutAct,
    SLOT(setEnabled(bool)));
  this->connect(this->TextArea->GetTextEdit(), SIGNAL(copyAvailable(bool)), this->copyAct,
    SLOT(setEnabled(bool)));
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createMenus()
{
  this->menuBar()->setObjectName("PythonScriptEditorMenuBar");
  this->fileMenu = menuBar()->addMenu(tr("&File"));
  this->fileMenu->setObjectName("File");
  this->fileMenu->addAction(this->TextArea->GetNewFileAction());
  this->fileMenu->addAction(this->TextArea->GetOpenFileAction());
  this->fileMenu->addAction(this->TextArea->GetSaveFileAction());
  this->fileMenu->addAction(this->TextArea->GetSaveFileAsAction());
  this->fileMenu->addAction(this->TextArea->GetSaveFileAsMacroAction());
  this->fileMenu->addSeparator();
  this->fileMenu->addAction(this->exitAct);

  this->editMenu = menuBar()->addMenu(tr("&Edit"));
  this->editMenu->setObjectName("Edit");
  this->editMenu->addAction(this->cutAct);
  this->editMenu->addAction(this->copyAct);
  this->editMenu->addAction(this->pasteAct);
  this->editMenu->addSeparator();
  this->editMenu->addAction(this->TextArea->GetUndoAction());
  this->editMenu->addAction(this->TextArea->GetRedoAction());

  this->menuBar()->addSeparator();
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createStatusBar()
{
  this->statusBar()->showMessage(tr("Ready"));
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setText(const QString& text)
{
  this->TextArea->GetTextEdit()->setPlainText(text);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::scrollToBottom()
{
  this->TextArea->GetTextEdit()->verticalScrollBar()->setValue(
    this->TextArea->GetTextEdit()->verticalScrollBar()->maximum());
}
