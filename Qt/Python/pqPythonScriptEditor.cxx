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
#include "pqPythonManager.h"
#include "pqPythonSyntaxHighlighter.h"
#include "pqPythonTabWidget.h"
#include "pqPythonTextArea.h"
#include "pqSettings.h"

#include <QAction>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QScrollBar>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>

#include <vtkPythonInterpreter.h>

//-----------------------------------------------------------------------------
pqPythonScriptEditor::pqPythonScriptEditor(QWidget* p)
  : QMainWindow(p)
  , TabWidget(new pqPythonTabWidget(this))
  , pythonManager(nullptr)
{
  this->setCentralWidget(TabWidget);

  this->createMenus();

  this->resize(300, 450);

  pqApplicationCore::instance()->settings()->restoreState("PythonScriptEditor", *this);
  vtkPythonInterpreter::Initialize();

  this->setWindowTitle(tr("ParaView Python Script Editor"));

  this->connect(
    this->TabWidget, &pqPythonTabWidget::fileSavedAsMacro, [this](const QString& filename) {
      if (pythonManager)
      {
        pythonManager->updateMacroList();
      }

      this->setWindowTitle(
        tr("%1[*] - %2").arg(details::stripFilename(filename)).arg(tr("Script Editor")));
      this->statusBar()->showMessage(tr("File %1 saved as macro").arg(filename), 4000);
    });

  this->connect(this->TabWidget, &pqPythonTabWidget::fileSaved, [this](const QString& filename) {
    this->setWindowTitle(
      tr("%1[*] - %2").arg(details::stripFilename(filename)).arg(tr("Script Editor")));
    this->statusBar()->showMessage(tr("File %1 saved").arg(filename), 4000);
  });

  this->connect(this->TabWidget, &pqPythonTabWidget::fileOpened, [this](const QString& filename) {
    this->setWindowTitle(
      tr("%1[*] - %2").arg(details::stripFilename(filename)).arg(tr("Script Editor")));
    this->statusBar()->showMessage(tr("File %1 opened").arg(filename), 4000);
  });

  connect(this->TabWidget, &QTabWidget::currentChanged,
    [this]() { this->TabWidget->updateActions(this->Actions); });

  this->TabWidget->connectActions(this->Actions);
  this->TabWidget->updateActions(this->Actions);
  pqPythonEditorActions::connect(this->Actions, this);

  this->createStatusBar();
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::closeEvent(QCloseEvent* e)
{
  if (this->TabWidget->saveOnClose())
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
  this->TabWidget->addNewTextArea(filename);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setSaveDialogDefaultDirectory(const QString& dir)
{
  this->TabWidget->getCurrentTextArea()->setDefaultSaveDirectory(dir);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setPythonManager(pqPythonManager* manager)
{
  this->pythonManager = manager;
  this->Actions[pqPythonEditorActions::Action::SaveFileAsMacro].setEnabled(manager != nullptr);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createMenus()
{
  using Action = pqPythonEditorActions::Action;

  this->menuBar()->setObjectName("PythonScriptEditorMenuBar");

  this->fileMenu = menuBar()->addMenu(tr("&File"));
  this->fileMenu->setObjectName("File");
  this->fileMenu->addAction(&this->Actions[Action::NewFile]);
  this->fileMenu->addAction(&this->Actions[Action::OpenFile]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFile]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFileAs]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFileAsMacro]);
  this->fileMenu->addSeparator();
  this->fileMenu->addAction(&this->Actions[Action::CloseCurrentTab]);
  this->fileMenu->addAction(&this->Actions[Action::Exit]);

  this->editMenu = menuBar()->addMenu(tr("&Edit"));
  this->editMenu->setObjectName("Edit");
  this->editMenu->addAction(&this->Actions[Action::Cut]);
  this->editMenu->addAction(&this->Actions[Action::Copy]);
  this->editMenu->addAction(&this->Actions[Action::Paste]);
  this->editMenu->addSeparator();
  this->editMenu->addAction(&this->Actions[Action::Undo]);
  this->editMenu->addAction(&this->Actions[Action::Redo]);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createStatusBar()
{
  this->statusBar()->showMessage(tr("Ready"));
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::scrollToBottom()
{
  this->TabWidget->getCurrentTextArea()->getTextEdit()->verticalScrollBar()->setValue(
    this->TabWidget->getCurrentTextArea()->getTextEdit()->verticalScrollBar()->maximum());
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::updateMacroList()
{
  auto editor = pqPythonScriptEditor::getUniqueInstance();
  if (editor->pythonManager)
  {
    editor->pythonManager->updateMacroList();
  }
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::updateTrace(const QString& str)
{
  this->TabWidget->updateTrace(str);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::stopTrace(const QString& str)
{
  this->TabWidget->stopTrace(str);
}
