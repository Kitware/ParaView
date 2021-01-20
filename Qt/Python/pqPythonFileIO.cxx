/*=========================================================================

   Program: ParaView
   Module:    pqPythonFileIO.cxx

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

#include "pqPythonFileIO.h"

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

#define Q_OPEN_FILE_(FILENAME, FLAGS)                                                              \
  QFile file(FILENAME);                                                                            \
  if (!file.open(FLAGS))                                                                           \
  {                                                                                                \
    QMessageBox::warning(nullptr, QString("Sorry!"),                                               \
      QString("Cannot write file %1:\n%2.").arg(FILENAME).arg(file.errorString()));                \
    return false;                                                                                  \
  }

//----------------------------------------------------------------------------
pqPythonFileIO::pqPythonFileIO(QWidget* parent, QTextEdit& text)
  : QObject(parent)
  , TextEdit(text)
{
  connect(&Actions[IOAction::NewFile], &QAction::triggered, this, &pqPythonFileIO::NewFile);
  connect(&Actions[IOAction::OpenFile], &QAction::triggered, this, &pqPythonFileIO::Open);
  connect(&Actions[IOAction::SaveFile], &QAction::triggered, this, &pqPythonFileIO::Save);
  connect(&Actions[IOAction::SaveFileAs], &QAction::triggered, this, &pqPythonFileIO::SaveAs);
  connect(
    &Actions[IOAction::SaveFileAsMacro], &QAction::triggered, this, &pqPythonFileIO::SaveAsMacro);
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::SaveOnClose()
{
  if (this->NeedSave())
  {
    const QMessageBox::StandardButton ret =
      QMessageBox::warning(nullptr, tr("Script Editor"), tr("The document has been modified.\n"
                                                            "Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
    {
      return this->Save();
    }
    else if (ret == QMessageBox::Cancel)
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::OpenFile(const QString& filename)
{
  if (!this->SaveOnClose())
  {
    return false;
  }

  return this->LoadFile(filename);
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::NewFile()
{
  if (!this->SaveOnClose())
  {
    return false;
  }

  this->TextEdit.clear();
  this->Filename.clear();
  this->SetModified(false);
  this->BufferErased();

  return true;
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::Open()
{
  if (!this->SaveOnClose())
  {
    return false;
  }

  const QString filename = pqFileDialog::getSaveFileName(nullptr, pqCoreUtilities::mainWidget(),
    tr("Save File"), this->DefaultSaveDirectory, tr("Python Script (*.py)"));
  if (filename.isEmpty() || !filename.endsWith(".py"))
  {
    return false;
  }

  return this->LoadFile(filename);
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::Save()
{
  if (this->Filename.isEmpty())
  {
    return this->SaveAs();
  }
  else
  {
    return this->SaveFile(this->Filename);
  }
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::SaveAs()
{
  QString filename = pqFileDialog::getSaveFileName(nullptr, pqCoreUtilities::mainWidget(),
    tr("Save File As"), this->DefaultSaveDirectory, tr("Python Script (*.py)"));

  if (filename.isEmpty())
  {
    return false;
  }

  if (!filename.endsWith(".py"))
  {
    filename.append(".py");
  }

  return this->SaveFile(filename);
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::SaveAsMacro()
{
  const QString userMacroDir = pqCoreUtilities::getParaViewUserDirectory() + "/Macros";
  const QDir existCheck(userMacroDir);
  if (!existCheck.exists() && !existCheck.mkpath(userMacroDir))
  {
    QMessageBox::warning(nullptr, QString("Sorry!"),
      QString("Could not create user Macro directory: %1.").arg(userMacroDir));
    return false;
  }

  const QString filename = pqFileDialog::getSaveFileName(nullptr, pqCoreUtilities::mainWidget(),
    tr("Save As Macro"), userMacroDir, tr("Python script (*.py)"));

  if (!filename.isEmpty() && this->SaveFile(filename))
  {
    this->FileSavedAsMacro(filename);
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::LoadFile(const QString& filename)
{
  Q_OPEN_FILE_(filename, QFile::ReadOnly | QFile::Text);

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  this->TextEdit.setPlainText(in.readAll());
  QApplication::restoreOverrideCursor();

  this->Filename = filename;

  this->SetModified(false);

  this->FileOpened(filename);

  this->BufferErased();

  return true;
}

//----------------------------------------------------------------------------
bool pqPythonFileIO::SaveFile(const QString& filename)
{
  Q_OPEN_FILE_(filename, QFile::WriteOnly | QFile::Text);

  QTextStream out(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  out << this->TextEdit.toPlainText();
  QApplication::restoreOverrideCursor();

  this->Filename = filename;

  this->SetModified(false);

  this->FileSaved(filename);

  return true;
}
