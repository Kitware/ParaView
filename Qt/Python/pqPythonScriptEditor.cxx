// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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

namespace details
{
void FillMenu(QMenu* menu, std::vector<QAction*>& actions)
{
  menu->clear();
  for (auto const& action : actions)
  {
    menu->addAction(action);
  }
}
}

pqPythonScriptEditor* pqPythonScriptEditor::UniqueInstance = nullptr;

//-----------------------------------------------------------------------------
pqPythonScriptEditor::pqPythonScriptEditor(QWidget* p)
  : QMainWindow(p)
  , TabWidget(new pqPythonTabWidget(this))
  , PythonManager(nullptr)
{
  this->setCentralWidget(TabWidget);

  this->createMenus();

  this->resize(300, 450);

  pqApplicationCore::instance()->settings()->restoreState("PythonScriptEditor", *this);
  vtkPythonInterpreter::Initialize();

  this->setWindowTitle(tr("ParaView Python Script Editor"));

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
void pqPythonScriptEditor::runCurrentTab()
{
  if (!this->TabWidget->getCurrentTextArea()->isEmpty())
  {
    const QString& code{ this->TabWidget->getCurrentTextArea()->getTextEdit()->toPlainText() };
    this->PythonManager->executeCode(code.toUtf8());
    pqApplicationCore::instance()->render();
  }
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
void pqPythonScriptEditor::load(const QString& filename)
{
  this->TabWidget->loadFile(filename);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setSaveDialogDefaultDirectory(const QString& dir)
{
  this->TabWidget->getCurrentTextArea()->setDefaultSaveDirectory(dir);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::setPythonManager(pqPythonManager* manager)
{
  this->PythonManager = manager;
  this->Actions[pqPythonEditorActions::GeneralActionType::SaveFileAsMacro].setEnabled(
    manager != nullptr);
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::createMenus()
{
  using Action = pqPythonEditorActions::GeneralActionType;

  this->menuBar()->setObjectName("PythonScriptEditorMenuBar");

  this->fileMenu = menuBar()->addMenu(tr("&File"));
  this->fileMenu->setObjectName("File");
  this->fileMenu->setToolTipsVisible(true);
  this->fileMenu->addAction(&this->Actions[Action::NewFile]);
  this->fileMenu->addAction(&this->Actions[Action::OpenFile]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFile]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFileAs]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFileAsMacro]);
  this->fileMenu->addAction(&this->Actions[Action::SaveFileAsScript]);
  this->fileMenu->addAction(&this->Actions[Action::Run]);
  this->fileMenu->addSeparator();
  this->fileMenu->addAction(&this->Actions[Action::CloseCurrentTab]);
  this->fileMenu->addAction(&this->Actions[Action::Exit]);

  this->editMenu = menuBar()->addMenu(tr("&Edit"));
  this->editMenu->setObjectName("Edit");
  this->editMenu->setToolTipsVisible(true);
  this->editMenu->addAction(&this->Actions[Action::Cut]);
  this->editMenu->addAction(&this->Actions[Action::Copy]);
  this->editMenu->addAction(&this->Actions[Action::Paste]);
  this->editMenu->addSeparator();
  this->editMenu->addAction(&this->Actions[Action::Undo]);
  this->editMenu->addAction(&this->Actions[Action::Redo]);

  this->Actions.updateScriptsList(this->PythonManager);
  auto menu = menuBar()->addMenu(tr("&Scripts"));
  menu->setToolTipsVisible(true);
  menu->addAction(&this->Actions[Action::SaveFileAsScript]);
  menu->addSeparator();

  this->scriptMenus[ScriptActionType::Open] = menu->addMenu(tr("Open..."));
  this->scriptMenus[ScriptActionType::Open]->setStatusTip(tr("Open a python script in a new tab"));
  this->scriptMenus[ScriptActionType::Open]->setToolTipsVisible(true);

  this->scriptMenus[ScriptActionType::Load] =
    menu->addMenu(tr("Load script into current editor tab..."));
  this->scriptMenus[ScriptActionType::Load]->setStatusTip(
    tr("Load a python script in the current opened tab and override its content"));
  this->scriptMenus[ScriptActionType::Load]->setToolTipsVisible(true);

  this->scriptMenus[ScriptActionType::Delete] = menu->addMenu(tr("Delete..."));
  this->scriptMenus[ScriptActionType::Delete]->setStatusTip(tr("Delete the script"));
  this->scriptMenus[ScriptActionType::Delete]->setToolTipsVisible(true);

  this->scriptMenus[ScriptActionType::Run] = menu->addMenu(tr("Run..."));
  this->scriptMenus[ScriptActionType::Run]->setStatusTip(
    tr("Load a python script in a new tab and run it"));
  this->scriptMenus[ScriptActionType::Run]->setToolTipsVisible(true);

  menu->addSeparator();
  menu->addAction(&this->Actions[Action::DeleteAll]);

  this->Actions.FillQMenu(this->scriptMenus);
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
  if (editor->PythonManager)
  {
    editor->PythonManager->updateMacroList();
  }
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::updateScriptList()
{
  auto editor = pqPythonScriptEditor::getUniqueInstance();
  editor->Actions.updateScriptsList(editor->PythonManager);
  editor->Actions.FillQMenu(editor->scriptMenus);
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

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::linkTo(QTextEdit* obj)
{
  auto instance = pqPythonScriptEditor::getUniqueInstance();
  instance->TabWidget->linkTo(obj);
}

//-----------------------------------------------------------------------------
pqPythonScriptEditor* pqPythonScriptEditor::getUniqueInstance()
{
  if (!pqPythonScriptEditor::UniqueInstance)
  {
    pqPythonScriptEditor::UniqueInstance = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
  }
  return pqPythonScriptEditor::UniqueInstance;
}

//-----------------------------------------------------------------------------
void pqPythonScriptEditor::bringFront()
{
  pqPythonScriptEditor* instance = pqPythonScriptEditor::getUniqueInstance();
  instance->show();
  instance->raise();
}

//-----------------------------------------------------------------------------
QString pqPythonScriptEditor::getMacrosDir()
{
  return pqCoreUtilities::getParaViewUserDirectory() + "/Macros";
}

//-----------------------------------------------------------------------------
QString pqPythonScriptEditor::getScriptsDir()
{
  return pqCoreUtilities::getParaViewUserDirectory() + "/Scripts";
}
