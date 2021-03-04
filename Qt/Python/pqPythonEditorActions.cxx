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

#include <QFileDialog>
#include <QKeySequence>
#include <QString>
#include <QTextEdit>
#include <QUndoStack>

//-----------------------------------------------------------------------------
pqPythonEditorActions::pqPythonEditorActions()
{
  this->Actions[Action::NewFile].setText(QObject::tr("&New"));
  this->Actions[Action::NewFile].setShortcut(QKeySequence::New);
  this->Actions[Action::NewFile].setStatusTip(QObject::tr("Create a new file"));

  this->Actions[Action::OpenFile].setText(QObject::tr("&Open..."));
  this->Actions[Action::OpenFile].setShortcut(QKeySequence::Open);
  this->Actions[Action::OpenFile].setStatusTip(QObject::tr("Open an existing file"));

  this->Actions[Action::SaveFile].setText(QObject::tr("&Save"));
  this->Actions[Action::SaveFile].setShortcut(QKeySequence::Save);
  this->Actions[Action::SaveFile].setStatusTip(QObject::tr("Save the document to disk"));

  this->Actions[Action::SaveFileAs].setText(QObject::tr("Save &As..."));
  this->Actions[Action::SaveFileAs].setStatusTip(QObject::tr("Save the document under a new name"));

  this->Actions[Action::SaveFileAsMacro].setText(QObject::tr("Save As &Macro..."));
  this->Actions[Action::SaveFileAsMacro].setStatusTip(QObject::tr("Save the document as a Macro"));

  this->Actions[Action::Cut].setText(QObject::tr("Cut"));
  this->Actions[Action::Cut].setShortcut(QKeySequence::Cut);
  this->Actions[Action::Cut].setStatusTip(QObject::tr("Cut the current selection's contents to the "
                                                      "clipboard"));

  this->Actions[Action::Undo].setText(QObject::tr("Undo"));
  this->Actions[Action::Undo].setShortcut(QKeySequence::Undo);
  this->Actions[Action::Undo].setStatusTip(QObject::tr("Undo the last edit of the file"));

  this->Actions[Action::Redo].setText(QObject::tr("Redo"));
  this->Actions[Action::Redo].setShortcut(QKeySequence::Redo);
  this->Actions[Action::Redo].setStatusTip(QObject::tr("Redo the last undo of the file"));

  this->Actions[Action::Copy].setText(QObject::tr("Copy"));
  this->Actions[Action::Copy].setShortcut(QKeySequence::Copy);
  this->Actions[Action::Copy].setStatusTip(
    QObject::tr("Copy the current selection's contents to the "
                "clipboard"));

  this->Actions[Action::Paste].setText(QObject::tr("Paste"));
  this->Actions[Action::Paste].setShortcut(QKeySequence::Paste);
  this->Actions[Action::Paste].setStatusTip(
    QObject::tr("Paste the clipboard's contents into the current "
                "selection"));

  this->Actions[Action::Exit].setText(QObject::tr("C&lose"));
  this->Actions[Action::Exit].setShortcut(QKeySequence::Quit);
  this->Actions[Action::Exit].setStatusTip(QObject::tr("Close the script editor"));

  this->Actions[Action::CloseCurrentTab].setText(QObject::tr("Close Current Tab"));
  this->Actions[Action::CloseCurrentTab].setShortcut(QKeySequence::Close);
  this->Actions[Action::CloseCurrentTab].setStatusTip(QObject::tr("Close the current opened tab"));
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonFileIO>(
  pqPythonEditorActions& a, pqPythonFileIO* fileIO)
{
  using Action = pqPythonEditorActions::Action;

  QObject::connect(&a[Action::SaveFile], &QAction::triggered, fileIO, &pqPythonFileIO::save);
  QObject::connect(&a[Action::SaveFileAs], &QAction::triggered, fileIO, &pqPythonFileIO::saveAs);
  QObject::connect(
    &a[Action::SaveFileAsMacro], &QAction::triggered, fileIO, &pqPythonFileIO::saveAsMacro);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonFileIO>(
  pqPythonEditorActions& actions, pqPythonFileIO* fileIO)
{
  Q_UNUSED(fileIO)

  using Action = pqPythonEditorActions::Action;

  actions[Action::SaveFile].disconnect();
  actions[Action::SaveFileAs].disconnect();
  actions[Action::SaveFileAsMacro].disconnect();
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonScriptEditor>(
  pqPythonEditorActions& actions, pqPythonScriptEditor* scriptEditor)
{
  using Action = pqPythonEditorActions::Action;
  QObject::connect(
    &actions[Action::Exit], &QAction::triggered, scriptEditor, &pqPythonScriptEditor::close);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonScriptEditor>(
  pqPythonEditorActions& actions, pqPythonScriptEditor* scriptEditor)
{
  Q_UNUSED(scriptEditor)

  using Action = pqPythonEditorActions::Action;
  actions[Action::Exit].disconnect();
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<QUndoStack>(
  pqPythonEditorActions& actions, QUndoStack* undoStack)
{
  using Action = pqPythonEditorActions::Action;
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
  using Action = pqPythonEditorActions::Action;

  actions[Action::Undo].disconnect();
  actions[Action::Redo].disconnect();

  undoStack->disconnect(&actions[Action::Undo]);
  undoStack->disconnect(&actions[Action::Redo]);
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<QTextEdit>(pqPythonEditorActions& actions, QTextEdit* textEdit)
{
  using Action = pqPythonEditorActions::Action;

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
  using Action = pqPythonEditorActions::Action;

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
  using Action = pqPythonEditorActions::Action;
  QObject::connect(&actions[Action::Undo], SIGNAL(triggered()), tArea, SLOT(update()));
  QObject::connect(&actions[Action::Redo], SIGNAL(triggered()), tArea, SLOT(update()));
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::disconnect<pqPythonTextArea>(
  pqPythonEditorActions& actions, pqPythonTextArea* tArea)
{
  Q_UNUSED(tArea)

  using Action = pqPythonEditorActions::Action;
  actions[Action::Undo].disconnect();
  actions[Action::Redo].disconnect();
}

//-----------------------------------------------------------------------------
template <>
void pqPythonEditorActions::connect<pqPythonTabWidget>(
  pqPythonEditorActions& actions, pqPythonTabWidget* tWidget)
{
  using Action = pqPythonEditorActions::Action;
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

  using Action = pqPythonEditorActions::Action;
  actions[Action::NewFile].disconnect();
  actions[Action::OpenFile].disconnect();
  actions[Action::CloseCurrentTab].disconnect();
}
