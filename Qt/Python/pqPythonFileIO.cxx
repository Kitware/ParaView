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
#include "pqPythonScriptEditor.h"

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QMessageBox>
#include <QTextEdit>
#include <QTextStream>

#define Q_OPEN_FILE_(FILENAME, FLAGS)                                                              \
  QFile file(FILENAME);                                                                            \
  if (!file.open(FLAGS))                                                                           \
  {                                                                                                \
    QMessageBox::warning(pqCoreUtilities::mainWidget(), QString("Sorry!"),                         \
      QString("Cannot open file %1:\n%2.").arg(FILENAME).arg(file.errorString()));                 \
    return false;                                                                                  \
  }

namespace details
{
//-----------------------------------------------------------------------------
QString GetSwapDir()
{
  const QString swapDir = pqCoreUtilities::getParaViewUserDirectory() + "/PythonSwap";
  const QDir existCheck(swapDir);
  if (!existCheck.mkpath(swapDir))
  {
    QMessageBox::warning(nullptr, QString("Sorry!"),
      QString("Could not create user PythonSwap directory: %1.").arg(swapDir));

    return {};
  }

  return swapDir;
}

//-----------------------------------------------------------------------------
QString GetSwapFilename(const QString& filepath)
{
  const QString swapFolder = GetSwapDir();
  if (!swapFolder.isEmpty())
  {
    return swapFolder + "/" + QFileInfo(filepath).fileName() + "." +
      QString::number(qHash(filepath));
  }

  return QString();
}

//-----------------------------------------------------------------------------
bool Write(const QString& filename, const QString& content)
{
  Q_OPEN_FILE_(filename, QFile::WriteOnly | QFile::Text);

  QTextStream out(&file);
  out << content;

  return true;
}
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::PythonFile::writeToFile() const
{
  if (this->Name.isEmpty())
  {
    QMessageBox::warning(
      pqCoreUtilities::mainWidget(), QString("Error"), QString("No Filename Given!"));
    return false;
  }

  return details::Write(this->Name, this->Text->toPlainText());
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::PythonFile::readFromFile(QString& str) const
{
  if (this->Name.isEmpty())
  {
    QMessageBox::warning(
      pqCoreUtilities::mainWidget(), QString("Error"), QString("No Filename Given!"));
    return false;
  }

  const QString swapFilename = details::GetSwapFilename(this->Name);
  if (QFileInfo::exists(swapFilename))
  {
    const QMessageBox::StandardButton ret =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Script Editor"),
        tr("Paraview found an old automatic save file %1. Would you like to recover its content?")
          .arg(swapFilename),
        QMessageBox::Yes | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret)
    {
      case QMessageBox::Yes:
        QFile::remove(this->Name);
        QFile::copy(swapFilename, this->Name);
        QFile::remove(swapFilename);
        break;

      case QMessageBox::Discard:
        QFile::remove(swapFilename);
        break;

      case QMessageBox::Cancel:
      default:
        return false;
    }
  }

  Q_OPEN_FILE_(Name, QFile::ReadOnly | QFile::Text);

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  str = in.readAll();
  QApplication::restoreOverrideCursor();

  return true;
}

//-----------------------------------------------------------------------------
void pqPythonFileIO::PythonFile::start()
{
  QObject::connect(this->Text, &QTextEdit::textChanged, [this]() {
    if (this->Text)
    {
      const QString swapFilename = details::GetSwapFilename(this->Name);
      if (!swapFilename.isEmpty())
      {
        details::Write(swapFilename, this->Text->toPlainText());
      }
    }
  });
}

//-----------------------------------------------------------------------------
void pqPythonFileIO::PythonFile::removeSwap() const
{
  const QString swapFilename = details::GetSwapFilename(this->Name);
  if (QFile::exists(swapFilename))
  {
    QFile::remove(swapFilename);
  }
}

//-----------------------------------------------------------------------------
pqPythonFileIO::pqPythonFileIO(QWidget* parent, QTextEdit& text)
  : QObject(parent)
  , TextEdit(text)
{
}

//-----------------------------------------------------------------------------
pqPythonFileIO::~pqPythonFileIO()
{
  this->File.removeSwap();
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::isDirty() const
{
  return this->TextEdit.document()->isModified();
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveOnClose()
{
  if (this->isDirty())
  {
    const QMessageBox::StandardButton ret =
      QMessageBox::warning(nullptr, tr("Script Editor"), tr("The document has been modified.\n"
                                                            "Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret)
    {
      case QMessageBox::Save:
        if (this->save())
        {
          return true;
        }

        return false;
      case QMessageBox::Discard:
        return true;
        break;
      case QMessageBox::Cancel:
      default:
        return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::openFile(const QString& filename)
{
  if (!this->saveOnClose())
  {
    return false;
  }

  const PythonFile file(filename, &this->TextEdit);
  QString fileContent;
  if (!file.readFromFile(fileContent))
  {
    return false;
  }

  this->File = file;
  this->File.start();

  this->TextEdit.setPlainText(fileContent);
  this->setModified(false);
  this->fileOpened(filename);

  return true;
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::save()
{
  if (this->File.Name.isEmpty())
  {
    return this->saveAs();
  }
  else
  {
    return this->saveBuffer(this->File.Name);
  }
}

//-----------------------------------------------------------------------------
void pqPythonFileIO::setModified(bool modified)
{
  this->TextEdit.document()->setModified(modified);
  if (modified)
  {
    this->contentChanged();
  }
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveAs()
{
  QString filename =
    pqFileDialog::getSaveFileName(nullptr, pqPythonScriptEditor::getUniqueInstance(),
      tr("Save File As"), this->DefaultSaveDirectory, tr("Python Script (*.py)"));

  if (filename.isEmpty())
  {
    return false;
  }

  if (!filename.endsWith(".py"))
  {
    filename.append(".py");
  }

  return this->saveBuffer(filename);
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveAsMacro()
{
  const QString userMacroDir = pqCoreUtilities::getParaViewUserDirectory() + "/Macros";
  const QDir existCheck(userMacroDir);
  if (!existCheck.exists() && !existCheck.mkpath(userMacroDir))
  {
    QMessageBox::warning(nullptr, QString("Sorry!"),
      QString("Could not create user Macro directory: %1.").arg(userMacroDir));
    return false;
  }

  const QString filename =
    pqFileDialog::getSaveFileName(nullptr, pqPythonScriptEditor::getUniqueInstance(),
      tr("Save As Macro"), userMacroDir, tr("Python script (*.py)"));

  if (this->saveBuffer(filename))
  {
    pqPythonScriptEditor::updateMacroList();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveBuffer(const QString& filename)
{
  if (filename.isEmpty())
  {
    return false;
  }

  const PythonFile file(filename, &this->TextEdit);
  if (file.writeToFile())
  {
    if (file != this->File)
    {
      this->File.removeSwap();
      this->File = file;
      this->File.start();
      this->bufferErased();
    }

    this->setModified(false);
    this->fileSaved(file.Name);

    return true;
  }

  return false;
}
