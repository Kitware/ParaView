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
#include "pqSettings.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
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
  : Superclass(p)
{
  this->pythonManager = NULL;
  this->TextEdit = new QTextEdit;
  this->TextEdit->setTabStopWidth(4);
  this->setCentralWidget(this->TextEdit);
  this->createActions();
  this->createMenus();
  this->createStatusBar();
  this->DefaultSaveDirectory = QDir::homePath();
  this->setCurrentFile("");
  this->connect(
    this->TextEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
  this->resize(300, 450);
  pqApplicationCore::instance()->settings()->restoreState("PythonScriptEditor", *this);
  vtkPythonInterpreter::Initialize();
  this->SyntaxHighlighter = new pqPythonSyntaxHighlighter(this->TextEdit, this);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::closeEvent(QCloseEvent* e)
{
  if (this->maybeSave())
  {
    this->TextEdit->clear();
    this->TextEdit->document()->setModified(false);
    this->setWindowModified(false);
    e->accept();
    pqApplicationCore::instance()->settings()->saveState(*this, "PythonScriptEditor");
  }
  else
  {
    e->ignore();
  }
}

//-----------------------------------------------------------------------------
bool pqPythonScriptEditor::newFile()
{
  if (this->maybeSave())
  {
    this->TextEdit->clear();
    this->setCurrentFile("");
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::open()
{
  if (this->maybeSave())
  {
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
      this->loadFile(fileName);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::open(const QString& fileName)
{
  if (this->maybeSave())
  {
    if (!fileName.isEmpty())
    {
      this->loadFile(fileName);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setSaveDialogDefaultDirectory(const QString& dir)
{
  this->DefaultSaveDirectory = dir;
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setPythonManager(pqPythonManager* manager)
{
  this->pythonManager = manager;
  this->saveAsMacroAct->setEnabled(manager);
}

//-----------------------------------------------------------------------------
bool pqPythonScriptEditor::save()
{
  if (this->CurrentFile.isEmpty())
  {
    return this->saveAs();
  }
  else
  {
    return this->saveFile(this->CurrentFile);
  }
}

//-----------------------------------------------------------------------------
bool pqPythonScriptEditor::saveAsMacro()
{
  QString userMacroDir = pqCoreUtilities::getParaViewUserDirectory() + "/Macros";
  QDir existCheck(userMacroDir);
  if (!existCheck.exists() && !existCheck.mkpath(userMacroDir))
  {
    qWarning() << "Could not create user Macro directory:" << userMacroDir;
    return false;
  }

  QString fileName = pqFileDialog::getSaveFileName(
    NULL, this, tr("Save Macro"), userMacroDir, tr("Python script (*.py)"));
  if (!fileName.isEmpty() && this->saveFile(fileName))
  {
    if (pythonManager)
    {
      pythonManager->updateMacroList();
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqPythonScriptEditor::saveAs()
{
  QString fileName = pqFileDialog::getSaveFileName(
    NULL, this, tr("Save File"), this->DefaultSaveDirectory, tr("Python script (*.py)"));
  if (fileName.isEmpty())
  {
    return false;
  }
  if (!fileName.endsWith(".py"))
  {
    fileName.append(".py");
  }
  return this->saveFile(fileName);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::documentWasModified()
{
  this->setWindowModified(this->TextEdit->document()->isModified());
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createActions()
{
  this->newAct = new QAction(tr("&New"), this);
  this->newAct->setShortcut(tr("Ctrl+N"));
  this->newAct->setStatusTip(tr("Create a new file"));
  this->connect(this->newAct, SIGNAL(triggered()), this, SLOT(newFile()));

  this->openAct = new QAction(tr("&Open..."), this);
  this->openAct->setShortcut(tr("Ctrl+O"));
  this->openAct->setStatusTip(tr("Open an existing file"));
  this->connect(this->openAct, SIGNAL(triggered()), this, SLOT(open()));

  this->saveAct = new QAction(tr("&Save"), this);
  this->saveAct->setShortcut(tr("Ctrl+S"));
  this->saveAct->setStatusTip(tr("Save the document to disk"));
  this->connect(this->saveAct, SIGNAL(triggered()), this, SLOT(save()));

  this->saveAsAct = new QAction(tr("Save &As..."), this);
  this->saveAsAct->setStatusTip(tr("Save the document under a new name"));
  this->connect(this->saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

  this->saveAsMacroAct = new QAction(tr("Save As &Macro..."), this);
  this->saveAsMacroAct->setStatusTip(tr("Save the document as a Macro"));
  this->connect(this->saveAsMacroAct, SIGNAL(triggered()), this, SLOT(saveAsMacro()));

  this->exitAct = new QAction(tr("C&lose"), this);
  this->exitAct->setShortcut(tr("Ctrl+W"));
  this->exitAct->setStatusTip(tr("Close the script editor"));
  this->connect(this->exitAct, SIGNAL(triggered()), this, SLOT(close()));

  this->cutAct = new QAction(tr("Cu&t"), this);
  this->cutAct->setShortcut(tr("Ctrl+X"));
  this->cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                                "clipboard"));
  this->connect(this->cutAct, SIGNAL(triggered()), this->TextEdit, SLOT(cut()));

  this->copyAct = new QAction(tr("&Copy"), this);
  this->copyAct->setShortcut(tr("Ctrl+C"));
  this->copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                                 "clipboard"));
  this->connect(this->copyAct, SIGNAL(triggered()), this->TextEdit, SLOT(copy()));

  this->pasteAct = new QAction(tr("&Paste"), this);
  this->pasteAct->setShortcut(tr("Ctrl+V"));
  this->pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                                  "selection"));
  this->connect(this->pasteAct, SIGNAL(triggered()), this->TextEdit, SLOT(paste()));

  this->saveAsMacroAct->setEnabled(false);
  this->cutAct->setEnabled(false);
  this->copyAct->setEnabled(false);
  this->connect(this->TextEdit, SIGNAL(copyAvailable(bool)), this->cutAct, SLOT(setEnabled(bool)));
  this->connect(this->TextEdit, SIGNAL(copyAvailable(bool)), this->copyAct, SLOT(setEnabled(bool)));
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createMenus()
{
  this->menuBar()->setObjectName("PythonScriptEditorMenuBar");
  this->fileMenu = menuBar()->addMenu(tr("&File"));
  this->fileMenu->setObjectName("File");
  this->fileMenu->addAction(this->newAct);
  this->fileMenu->addAction(this->openAct);
  this->fileMenu->addAction(this->saveAct);
  this->fileMenu->addAction(this->saveAsAct);
  this->fileMenu->addAction(this->saveAsMacroAct);
  this->fileMenu->addSeparator();
  this->fileMenu->addAction(this->exitAct);

  this->editMenu = menuBar()->addMenu(tr("&Edit"));
  this->editMenu->setObjectName("Edit");
  this->editMenu->addAction(this->cutAct);
  this->editMenu->addAction(this->copyAct);
  this->editMenu->addAction(this->pasteAct);

  this->menuBar()->addSeparator();
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createStatusBar()
{
  this->statusBar()->showMessage(tr("Ready"));
}

//-----------------------------------------------------------------------------
bool pqPythonScriptEditor::maybeSave()
{
  if (this->TextEdit->document()->isModified())
  {
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Script Editor"), tr("The document has been modified.\n"
                                                             "Do you want to save your changes?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
    {
      return this->save();
    }
    else if (ret == QMessageBox::Cancel)
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setText(const QString& text)
{
  this->TextEdit->setPlainText(text);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::loadFile(const QString& fileName)
{
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text))
  {
    QMessageBox::warning(this, tr("Script Editor"),
      tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  this->TextEdit->setPlainText(in.readAll());
  QApplication::restoreOverrideCursor();

  this->setCurrentFile(fileName);
  this->statusBar()->showMessage(tr("File loaded"), 2000);
}

//-----------------------------------------------------------------------------
bool pqPythonScriptEditor::saveFile(const QString& fileName)
{
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QFile::Text))
  {
    QMessageBox::warning(
      this, tr("Sorry!"), tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return false;
  }

  QTextStream out(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  out << this->TextEdit->toPlainText();
  QApplication::restoreOverrideCursor();

  this->setCurrentFile(fileName);
  this->statusBar()->showMessage(tr("File saved"), 2000);
  emit this->fileSaved();
  return true;
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setCurrentFile(const QString& fileName)
{
  this->CurrentFile = fileName;
  this->TextEdit->document()->setModified(false);
  this->setWindowModified(false);

  QString shownName;
  if (this->CurrentFile.isEmpty())
  {
    shownName = "untitled.py";
  }
  else
  {
    shownName = strippedName(this->CurrentFile);
  }

  this->setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Script Editor")));
}

//-----------------------------------------------------------------------------
QString pqPythonScriptEditor::strippedName(const QString& fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::scrollToBottom()
{
  this->TextEdit->verticalScrollBar()->setValue(this->TextEdit->verticalScrollBar()->maximum());
}
