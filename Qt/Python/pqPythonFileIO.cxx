// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonFileIO.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPythonScriptEditor.h"
#include "pqScopedOverrideCursor.h"
#include "pqServer.h"

#include "vtkPVSession.h"
#include "vtkSMFileUtilities.h"
#include "vtkSMSessionProxyManager.h"

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QMessageBox>
#include <QTextEdit>
#include <QTextStream>

namespace
{
//-----------------------------------------------------------------------------
QString GetSwapDir()
{
  QString swapDir = pqCoreUtilities::getParaViewUserDirectory() + "/PythonSwap";
  const QDir existCheck(swapDir);
  if (!existCheck.mkpath(swapDir))
  {
    QMessageBox::warning(nullptr, QCoreApplication::translate("pqPythonFileIO", "Sorry!"),
      qPrintable(QCoreApplication::translate(
        "pqPythonFileIO", "Could not create user PythonSwap directory: %1.")
                   .arg(swapDir)));

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
//-----------------------------------------------------------------------------
QString Read(const QString& filename, vtkTypeUInt32 location)
{
  auto pxm = pqApplicationCore::instance()->getActiveServer()->proxyManager();
  return QString(pxm->LoadString(filename.toStdString().c_str(), location).c_str());
}

//-----------------------------------------------------------------------------
bool Write(const QString& filename, vtkTypeUInt32 location, const QString& content)
{
  auto pxm = pqApplicationCore::instance()->getActiveServer()->proxyManager();
  return pxm->SaveString(content.toStdString().c_str(), filename.toStdString().c_str(), location);
}
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::PythonFile::writeToFile() const
{
  if (this->Name.isEmpty())
  {
    QMessageBox::warning(pqCoreUtilities::mainWidget(),
      QCoreApplication::translate("pqPythonFileIO", "Error"),
      QCoreApplication::translate("pqPythonFileIO", "No Filename Given!"));
    return false;
  }

  return ::Write(this->Name, this->Location, this->Text->toPlainText());
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::PythonFile::readFromFile(QString& str) const
{
  if (this->Name.isEmpty())
  {
    QMessageBox::warning(pqCoreUtilities::mainWidget(),
      QCoreApplication::translate("pqPythonFileIO", "Error"),
      QCoreApplication::translate("pqPythonFileIO", "No Filename Given!"));
    return false;
  }

  const QString swapFilename = ::GetSwapFilename(this->Name);
  if (QFileInfo::exists(swapFilename))
  {
    const auto userAnswer = pqCoreUtilities::promptUserGeneric(tr("Script Editor"),
      tr("Paraview found an old automatic save file %1. Would you like to recover its content?"),
      QMessageBox::Warning, QMessageBox::Yes | QMessageBox::Discard | QMessageBox::Cancel, nullptr);
    switch (userAnswer)
    {
      case QMessageBox::Yes:
      {
        const auto contents = ::Read(swapFilename, vtkPVSession::CLIENT);
        ::Write(this->Name, this->Location, contents);
        QFile::remove(swapFilename);
        break;
      }
      case QMessageBox::Discard:
        QFile::remove(swapFilename);
        break;

      case QMessageBox::Cancel:
      default:
        return false;
    }
  }

  {
    pqScopedOverrideCursor scopedWaitCursor(Qt::WaitCursor);
    str = ::Read(this->Name, this->Location);
  }

  pqPythonScriptEditor::bringFront();

  return true;
}

//-----------------------------------------------------------------------------
void pqPythonFileIO::PythonFile::start()
{
  QObject::connect(this->Text, &QTextEdit::textChanged,
    [this]()
    {
      if (this->Text)
      {
        const QString swapFilename = ::GetSwapFilename(this->Name);
        if (!swapFilename.isEmpty())
        {
          ::Write(swapFilename, vtkPVSession::CLIENT, this->Text->toPlainText());
        }
      }
    });
}

//-----------------------------------------------------------------------------
void pqPythonFileIO::PythonFile::removeSwap() const
{
  const QString swapFilename = ::GetSwapFilename(this->Name);
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
    switch (pqCoreUtilities::promptUserGeneric(tr("Script Editor"),
      tr("The document has been modified.\n Do you want to save your changes?"),
      QMessageBox::Warning, QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
      nullptr))
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
bool pqPythonFileIO::openFile(const QString& filename, vtkTypeUInt32 location)
{
  if (!this->saveOnClose())
  {
    return false;
  }

  const PythonFile file(filename, location, &this->TextEdit);
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
    return this->saveBuffer(this->File.Name, this->File.Location);
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
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  auto filenameAndLocation = pqFileDialog::getSaveFileNameAndLocation(server,
    pqPythonScriptEditor::getUniqueInstance(), tr("Save File As"), this->DefaultSaveDirectory,
    tr("Python Files") + QString(" (*.py);;"), false, false);

  auto& fileName = filenameAndLocation.first;
  auto& location = filenameAndLocation.second;
  if (fileName.isEmpty())
  {
    return false;
  }

  if (!fileName.endsWith(".py"))
  {
    fileName.append(".py");
  }

  pqPythonScriptEditor::bringFront();

  return this->saveBuffer(fileName, location);
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveAsMacro()
{
  const QString userMacroDir = pqPythonScriptEditor::getMacrosDir();
  const QDir existCheck(userMacroDir);
  if (!existCheck.exists() && !existCheck.mkpath(userMacroDir))
  {
    QMessageBox::warning(nullptr, tr("Sorry!"),
      qPrintable(tr("Could not create user Macro directory: %1.").arg(userMacroDir)));
    return false;
  }

  const QString filename =
    pqFileDialog::getSaveFileName(nullptr, pqPythonScriptEditor::getUniqueInstance(),
      tr("Save As Macro"), userMacroDir, tr("Python Files") + QString(" (*.py);;"));

  pqPythonScriptEditor::bringFront();

  if (this->saveBuffer(filename, vtkPVSession::CLIENT))
  {
    pqPythonScriptEditor::updateMacroList();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveAsScript()
{
  const QString userScriptDir = pqPythonScriptEditor::getScriptsDir();
  const QDir existCheck(userScriptDir);
  if (!existCheck.exists() && !existCheck.mkpath(userScriptDir))
  {
    QMessageBox::warning(nullptr, tr("Sorry!"),
      qPrintable(tr("Could not create user Script directory: %1.").arg(userScriptDir)));
    return false;
  }

  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  const auto filenameAndLocation =
    pqFileDialog::getSaveFileNameAndLocation(server, pqPythonScriptEditor::getUniqueInstance(),
      tr("Save As Script"), userScriptDir, tr("Python Files") + QString(" (*.py);;"), false, false);

  pqPythonScriptEditor::bringFront();

  if (this->saveBuffer(filenameAndLocation.first, filenameAndLocation.second))
  {
    pqPythonScriptEditor::updateScriptList();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPythonFileIO::saveBuffer(const QString& filename, vtkTypeUInt32 location)
{
  if (filename.isEmpty())
  {
    return false;
  }

  const PythonFile file(filename, location, &this->TextEdit);
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
