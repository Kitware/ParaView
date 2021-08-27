/*=========================================================================

   Program: ParaView
   Module:    pqPythonEditorActions.cxx

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

#include "pqPythonEditorActions.h"

#include "pqFileDialog.h"
#include "pqPythonFileIO.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonTabWidget.h"
#include "pqPythonTextArea.h"
#include "pqQtDeprecated.h"

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

  this->GeneralActions[Action::NewFile].setText(QObject::tr("&New"));
  this->GeneralActions[Action::NewFile].setShortcut(QKeySequence::New);
  this->GeneralActions[Action::NewFile].setStatusTip(QObject::tr("Create a new file"));

  this->GeneralActions[Action::OpenFile].setText(QObject::tr("&Open..."));
  this->GeneralActions[Action::OpenFile].setShortcut(QKeySequence::Open);
  this->GeneralActions[Action::OpenFile].setStatusTip(QObject::tr("Open an existing file"));

  this->GeneralActions[Action::SaveFile].setText(QObject::tr("&Save"));
  this->GeneralActions[Action::SaveFile].setShortcut(QKeySequence::Save);
  this->GeneralActions[Action::SaveFile].setStatusTip(QObject::tr("Save the document to disk"));

  this->GeneralActions[Action::SaveFileAs].setText(QObject::tr("Save &As..."));
  this->GeneralActions[Action::SaveFileAs].setStatusTip(
    QObject::tr("Save the document under a new name"));

  this->GeneralActions[Action::SaveFileAsMacro].setText(QObject::tr("Save As &Macro..."));
  this->GeneralActions[Action::SaveFileAsMacro].setStatusTip(
    QObject::tr("Save the document as a Macro"));

  this->GeneralActions[Action::SaveFileAsScript].setText(QObject::tr("Save As &Script..."));
  this->GeneralActions[Action::SaveFileAsScript].setStatusTip(
    QObject::tr("Save the document as a Script"));

  this->GeneralActions[Action::Cut].setText(QObject::tr("Cut"));
  this->GeneralActions[Action::Cut].setShortcut(QKeySequence::Cut);
  this->GeneralActions[Action::Cut].setStatusTip(
    QObject::tr("Cut the current selection's contents to the "
                "clipboard"));

  this->GeneralActions[Action::Undo].setText(QObject::tr("Undo"));
  this->GeneralActions[Action::Undo].setShortcut(QKeySequence::Undo);
  this->GeneralActions[Action::Undo].setStatusTip(QObject::tr("Undo the last edit of the file"));

  this->GeneralActions[Action::Redo].setText(QObject::tr("Redo"));
  this->GeneralActions[Action::Redo].setShortcut(QKeySequence::Redo);
  this->GeneralActions[Action::Redo].setStatusTip(QObject::tr("Redo the last undo of the file"));

  this->GeneralActions[Action::Copy].setText(QObject::tr("Copy"));
  this->GeneralActions[Action::Copy].setShortcut(QKeySequence::Copy);
  this->GeneralActions[Action::Copy].setStatusTip(
    QObject::tr("Copy the current selection's contents to the "
                "clipboard"));

  this->GeneralActions[Action::Paste].setText(QObject::tr("Paste"));
  this->GeneralActions[Action::Paste].setShortcut(QKeySequence::Paste);
  this->GeneralActions[Action::Paste].setStatusTip(
    QObject::tr("Paste the clipboard's contents into the current "
                "selection"));

  this->GeneralActions[Action::Exit].setText(QObject::tr("C&lose"));
  this->GeneralActions[Action::Exit].setShortcut(QKeySequence::Quit);
  this->GeneralActions[Action::Exit].setStatusTip(QObject::tr("Close the script editor"));

  this->GeneralActions[Action::CloseCurrentTab].setText(QObject::tr("Close Current Tab"));
  this->GeneralActions[Action::CloseCurrentTab].setShortcut(QKeySequence::Close);
  this->GeneralActions[Action::CloseCurrentTab].setStatusTip(
    QObject::tr("Close the current opened tab"));
}

//-----------------------------------------------------------------------------
void pqPythonEditorActions::updateScriptsList()
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
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonScriptEditor>(
  pqPythonEditorActions& actions, pqPythonScriptEditor* scriptEditor)
{
  Q_UNUSED(scriptEditor)

  using Action = pqPythonEditorActions::GeneralActionType;

  actions[Action::Exit].disconnect();
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
    pqFileDialog dialog(nullptr, pqPythonScriptEditor::getUniqueInstance(),
      QObject::tr("Open File"), QString(), QObject::tr("Python Script (*.py)"));
    dialog.setObjectName("FileOpenDialog");
    if (QFileDialog::Accepted == dialog.exec())
    {
      QList<QStringList> files = dialog.getAllSelectedFiles();
      tWidget->addNewTextArea(files.begin()->first());
    }

    pqPythonScriptEditor::bringFront();
  });
  QObject::connect(&actions[Action::CloseCurrentTab], &QAction::triggered, tWidget,
    &pqPythonTabWidget::closeCurrentTab);
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
}
