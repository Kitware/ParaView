// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonEditorActions.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqPythonFileIO.h"
#include "pqPythonManager.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonTabWidget.h"
#include "pqPythonTextArea.h"
#include "pqQtDeprecated.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QKeySequence>
#include <QString>
#include <QTextEdit>
#include <QUndoStack>

namespace details
{
QStringList ListFiles(const QString& directory)
{
  const QDir dir(directory);
  return dir.entryList(QStringList() << "*.py", QDir::Files);
}
}

//-----------------------------------------------------------------------------
pqPythonEditorActions::pqPythonEditorActions()
{
  using Action = GeneralActionType;

  this->GeneralActions[Action::NewFile].setText(
    QCoreApplication::translate("pqPythonEditorActions", "&New"));
  this->GeneralActions[Action::NewFile].setShortcut(QKeySequence::New);
  this->GeneralActions[Action::NewFile].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Create a new file"));

  this->GeneralActions[Action::OpenFile].setText(
    QCoreApplication::translate("pqPythonEditorActions", "&Open..."));
  this->GeneralActions[Action::OpenFile].setShortcut(QKeySequence::Open);
  this->GeneralActions[Action::OpenFile].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Open an existing file"));

  this->GeneralActions[Action::SaveFile].setText(
    QCoreApplication::translate("pqPythonEditorActions", "&Save"));
  this->GeneralActions[Action::SaveFile].setShortcut(QKeySequence::Save);
  this->GeneralActions[Action::SaveFile].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Save the document to disk"));

  this->GeneralActions[Action::SaveFileAs].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Save &As..."));
  this->GeneralActions[Action::SaveFileAs].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Save the document under a new name"));

  this->GeneralActions[Action::SaveFileAsMacro].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Save As &Macro..."));
  this->GeneralActions[Action::SaveFileAsMacro].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Save the document as a Macro"));

  this->GeneralActions[Action::SaveFileAsScript].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Save As &Script..."));
  this->GeneralActions[Action::SaveFileAsScript].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Save the document as a Script"));

  this->GeneralActions[Action::DeleteAll].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Delete All"));
  this->GeneralActions[Action::DeleteAll].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Delete all scripts from disk"));

  this->GeneralActions[Action::Run].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Run..."));
  this->GeneralActions[Action::Run].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Run the currently edited script"));

  this->GeneralActions[Action::Cut].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Cut"));
  this->GeneralActions[Action::Cut].setShortcut(QKeySequence::Cut);
  this->GeneralActions[Action::Cut].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions",
      "Cut the current selection's contents to the "
      "clipboard"));

  this->GeneralActions[Action::Undo].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Undo"));
  this->GeneralActions[Action::Undo].setShortcut(QKeySequence::Undo);
  this->GeneralActions[Action::Undo].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Undo the last edit of the file"));

  this->GeneralActions[Action::Redo].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Redo"));
  this->GeneralActions[Action::Redo].setShortcut(QKeySequence::Redo);
  this->GeneralActions[Action::Redo].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Redo the last undo of the file"));

  this->GeneralActions[Action::Copy].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Copy"));
  this->GeneralActions[Action::Copy].setShortcut(QKeySequence::Copy);
  this->GeneralActions[Action::Copy].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions",
      "Copy the current selection's contents to the "
      "clipboard"));

  this->GeneralActions[Action::Paste].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Paste"));
  this->GeneralActions[Action::Paste].setShortcut(QKeySequence::Paste);
  this->GeneralActions[Action::Paste].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions",
      "Paste the clipboard's contents into the current "
      "selection"));

  this->GeneralActions[Action::Exit].setText(
    QCoreApplication::translate("pqPythonEditorActions", "C&lose"));
  this->GeneralActions[Action::Exit].setShortcut(QKeySequence::Quit);
  this->GeneralActions[Action::Exit].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Close the script editor"));

  this->GeneralActions[Action::CloseCurrentTab].setText(
    QCoreApplication::translate("pqPythonEditorActions", "Close Current Tab"));
  this->GeneralActions[Action::CloseCurrentTab].setShortcut(QKeySequence::Close);
  this->GeneralActions[Action::CloseCurrentTab].setStatusTip(
    QCoreApplication::translate("pqPythonEditorActions", "Close the current opened tab"));
}

//-----------------------------------------------------------------------------
void pqPythonEditorActions::updateScriptsList(pqPythonManager* python_mgr)
{
  auto scripts = details::ListFiles(pqPythonScriptEditor::getScriptsDir());

  this->ScriptActions.clear();
  this->ScriptActions.reserve(scripts.size());

  for (auto const& script : scripts)
  {
    const QString filename = script.split(".", PV_QT_SKIP_EMPTY_PARTS).at(0);
    this->ScriptActions.emplace_back(ScriptAction());
    const std::size_t position = this->ScriptActions.size() - 1;
    auto& actions = this->ScriptActions[position];

    QAction* openAction = &actions[ScriptAction::Type::Open];
    openAction->setText(filename);
    QObject::connect(openAction, &QAction::triggered, [openAction]() {
      const QString openedFilename =
        pqPythonScriptEditor::getScriptsDir() + "/" + openAction->text() + ".py";
      auto scriptEditor = pqPythonScriptEditor::getUniqueInstance();
      scriptEditor->open(openedFilename);
    });

    QAction* loadAction = &actions[ScriptAction::Type::Load];
    loadAction->setText(filename);
    QObject::connect(loadAction, &QAction::triggered, [loadAction]() {
      const QString loadedFilename =
        pqPythonScriptEditor::getScriptsDir() + "/" + loadAction->text() + ".py";
      auto scriptEditor = pqPythonScriptEditor::getUniqueInstance();
      scriptEditor->load(loadedFilename);
    });

    QAction* deleteAction = &actions[ScriptAction::Type::Delete];
    deleteAction->setText(filename);
    QObject::connect(deleteAction, &QAction::triggered, [deleteAction]() {
      const QString deletedFilename =
        pqPythonScriptEditor::getScriptsDir() + "/" + deleteAction->text() + ".py";
      QFile file(deletedFilename);
      file.remove();
      pqPythonScriptEditor::getUniqueInstance()->updateScriptList();
    });

    QAction* runAction = &actions[ScriptAction::Type::Run];
    runAction->setText(filename);
    QObject::connect(runAction, &QAction::triggered, [runAction, python_mgr]() {
      const QString runFilename =
        pqPythonScriptEditor::getScriptsDir() + "/" + runAction->text() + ".py";
      python_mgr->executeScriptAndRender(runFilename);
      auto scriptEditor = pqPythonScriptEditor::getUniqueInstance();
      scriptEditor->open(runFilename);
    });
  }
}

//-----------------------------------------------------------------------------
void pqPythonEditorActions::FillQMenu(EnumArray<ScriptAction::Type, QMenu*> menus)
{
  for (int j = 0; j < static_cast<int>(ScriptAction::Type::END); ++j)
  {
    const ScriptAction::Type type = static_cast<ScriptAction::Type>(j);
    menus[type]->clear();

    for (auto action : this->ScriptActions)
    {
      menus[type]->addAction(&action[type]);
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonFileIO>(
  pqPythonEditorActions& a, pqPythonFileIO* fileIO)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  QObject::connect(&a[Action::SaveFile], &QAction::triggered, fileIO, &pqPythonFileIO::save);
  QObject::connect(&a[Action::SaveFileAs], &QAction::triggered, fileIO, &pqPythonFileIO::saveAs);
  QObject::connect(
    &a[Action::SaveFileAsMacro], &QAction::triggered, fileIO, &pqPythonFileIO::saveAsMacro);
  QObject::connect(
    &a[Action::SaveFileAsScript], &QAction::triggered, fileIO, &pqPythonFileIO::saveAsScript);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonFileIO>(
  pqPythonEditorActions& actions, pqPythonFileIO* fileIO)
{
  Q_UNUSED(fileIO)

  using Action = pqPythonEditorActions::GeneralActionType;

  actions[Action::SaveFile].disconnect();
  actions[Action::SaveFileAs].disconnect();
  actions[Action::SaveFileAsMacro].disconnect();
  actions[Action::SaveFileAsScript].disconnect();
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonScriptEditor>(
  pqPythonEditorActions& actions, pqPythonScriptEditor* scriptEditor)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  QObject::connect(
    &actions[Action::Exit], &QAction::triggered, scriptEditor, &pqPythonScriptEditor::close);
  QObject::connect(
    &actions[Action::Run], &QAction::triggered, scriptEditor, &pqPythonScriptEditor::runCurrentTab);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonScriptEditor>(
  pqPythonEditorActions& actions, pqPythonScriptEditor* scriptEditor)
{
  Q_UNUSED(scriptEditor)

  using Action = pqPythonEditorActions::GeneralActionType;

  actions[Action::Exit].disconnect();
  actions[Action::Run].disconnect();
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<QUndoStack>(
  pqPythonEditorActions& actions, QUndoStack* undoStack)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  QAction::connect(&actions[Action::Undo], &QAction::triggered, undoStack, &QUndoStack::undo);
  QAction::connect(&actions[Action::Redo], &QAction::triggered, undoStack, &QUndoStack::redo);

  actions[Action::Undo].setEnabled(false);
  actions[Action::Redo].setEnabled(false);

  QObject::connect(
    undoStack, &QUndoStack::canUndoChanged, &actions[Action::Undo], &QAction::setEnabled);
  QObject::connect(
    undoStack, &QUndoStack::canRedoChanged, &actions[Action::Redo], &QAction::setEnabled);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<QUndoStack>(
  pqPythonEditorActions& actions, QUndoStack* undoStack)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  actions[Action::Undo].disconnect();
  actions[Action::Redo].disconnect();

  undoStack->disconnect(&actions[Action::Undo]);
  undoStack->disconnect(&actions[Action::Redo]);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<QTextEdit>(pqPythonEditorActions& actions, QTextEdit* textEdit)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  QObject::connect(&actions[Action::Cut], &QAction::triggered, textEdit, &QTextEdit::cut);
  QObject::connect(&actions[Action::Copy], &QAction::triggered, textEdit, &QTextEdit::copy);
  QObject::connect(&actions[Action::Paste], &QAction::triggered, textEdit, &QTextEdit::paste);

  actions[Action::Cut].setEnabled(false);
  actions[Action::Copy].setEnabled(false);

  QObject::connect(
    textEdit, &QTextEdit::copyAvailable, &actions[Action::Cut], &QAction::setEnabled);
  QObject::connect(
    textEdit, &QTextEdit::copyAvailable, &actions[Action::Copy], &QAction::setEnabled);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<QTextEdit>(
  pqPythonEditorActions& actions, QTextEdit* textEdit)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  actions[Action::Cut].disconnect();
  actions[Action::Copy].disconnect();
  actions[Action::Paste].disconnect();

  textEdit->disconnect(&actions[Action::Cut]);
  textEdit->disconnect(&actions[Action::Copy]);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonTextArea>(
  pqPythonEditorActions& actions, pqPythonTextArea* tArea)
{
  using Action = pqPythonEditorActions::GeneralActionType;
  QObject::connect(&actions[Action::Undo], SIGNAL(triggered()), tArea, SLOT(update()));
  QObject::connect(&actions[Action::Redo], SIGNAL(triggered()), tArea, SLOT(update()));
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonTextArea>(
  pqPythonEditorActions& actions, pqPythonTextArea* tArea)
{
  Q_UNUSED(tArea)

  using Action = pqPythonEditorActions::GeneralActionType;
  actions[Action::Undo].disconnect();
  actions[Action::Redo].disconnect();
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonTabWidget>(
  pqPythonEditorActions& actions, pqPythonTabWidget* tWidget)
{
  using Action = pqPythonEditorActions::GeneralActionType;

  QObject::connect(
    &actions[Action::NewFile], &QAction::triggered, tWidget, &pqPythonTabWidget::createNewEmptyTab);
  QObject::connect(&actions[Action::OpenFile], &QAction::triggered, [tWidget]() {
    pqServer* server = pqApplicationCore::instance()->getActiveServer();
    pqFileDialog dialog(server, pqPythonScriptEditor::getUniqueInstance(),
      QCoreApplication::translate("pqPythonEditorActions", "Open File"), QString(),
      QCoreApplication::translate("pqPythonEditorActions", "Python Files") + QString(" (*.py);;"),
      false, false);
    dialog.setObjectName("FileOpenDialog");
    if (QFileDialog::Accepted == dialog.exec())
    {
      const QString filename = dialog.getAllSelectedFiles().begin()->first();
      const vtkTypeUInt32 location = dialog.getSelectedLocation();
      tWidget->addNewTextArea(filename, location);
    }

    pqPythonScriptEditor::bringFront();
  });
  QObject::connect(&actions[Action::CloseCurrentTab], &QAction::triggered, tWidget,
    &pqPythonTabWidget::closeCurrentTab);
  QObject::connect(&actions[Action::DeleteAll], &QAction::triggered, tWidget, [tWidget]() {
    QMessageBox::StandardButton ret = QMessageBox::question(tWidget,
      QCoreApplication::translate("pqPythonEditorActions", "Delete All"),
      QCoreApplication::translate(
        "pqPythonEditorActions", "All scripts will be deleted. Are you sure?"));
    if (ret == QMessageBox::StandardButton::Yes)
    {
      pqCoreUtilities::removeRecursively(pqPythonScriptEditor::getScriptsDir());
      for (int i = tWidget->count() - 1; i >= 0; --i)
      {
        Q_EMIT tWidget->tabCloseRequested(i);
      }
      pqPythonScriptEditor::updateScriptList();
    }
  });
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonTabWidget>(
  pqPythonEditorActions& actions, pqPythonTabWidget* tWidget)
{
  Q_UNUSED(tWidget)

  using Action = pqPythonEditorActions::GeneralActionType;

  actions[Action::NewFile].disconnect();
  actions[Action::OpenFile].disconnect();
  actions[Action::CloseCurrentTab].disconnect();
  actions[Action::DeleteAll].disconnect();
}
