// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFileDialogEventPlayer.h"
#include "pqCoreTestUtility.h"
#include "pqQtDeprecated.h"

#include "pqEventDispatcher.h"
#include "pqFileDialog.h"

#include <vtksys/SystemTools.hxx>

#include <QApplication>
#include <QDir>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqFileDialogEventPlayer::pqFileDialogEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

//-----------------------------------------------------------------------------
bool pqFileDialogEventPlayer::playEvent(
  QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  // Handle playback for pqFileDialog and all its children ...
  pqFileDialog* object = nullptr;
  for (QObject* o = Object; o; o = o->parent())
  {
    if ((object = qobject_cast<pqFileDialog*>(o)))
    {
      break;
    }
  }
  if (!object)
  {
    return false;
  }

  QString fileString = Arguments;

  const QString data_directory = pqCoreTestUtility::DataRoot();
  if (fileString.contains("PARAVIEW_DATA_ROOT") && data_directory.isEmpty())
  {
    qCritical()
      << "You must set the PARAVIEW_DATA_ROOT environment variable to play-back file selections.";
    Error = true;
    return true;
  }

  const QString test_directory = pqCoreTestUtility::TestDirectory();
  if (fileString.contains("PARAVIEW_TEST_ROOT") && test_directory.isEmpty())
  {
    qCritical() << "You must specify --test-directory in the command line options.";
    Error = true;
    return true;
  }

  fileString.replace("$PARAVIEW_DATA_ROOT", data_directory);
  fileString.replace("$PARAVIEW_TEST_ROOT", test_directory);
  if (Command == "filesSelected")
  {
    if (object->selectFile(fileString))
    {
      pqEventDispatcher::processEventsAndWait(0);
    }
    else
    {
      qCritical() << "Dialog couldn't accept " << fileString;
      Error = true;
    }

    return true;
  }

  if (Command == "cancelled")
  {
    object->reject();
    return true;
  }
  if (Command == "remove")
  {
    // Delete the file.
    vtksys::SystemTools::RemoveFile(fileString.toUtf8().toStdString());
    return true;
  }
  if (Command == "copy")
  {
    QStringList parts = fileString.split(';', PV_QT_SKIP_EMPTY_PARTS);
    if (parts.size() != 2)
    {
      qCritical() << "Invalid argument to `copy`. Expecting paths separated by `;`.";
      Error = true;
    }
    if (!QFile::copy(parts[0], parts[1]))
    {
      qCritical() << "Failed to copy `" << parts[0] << "` to `" << parts[1] << "`.";
      Error = true;
    }
    return true;
  }
  if (Command == "removeDir")
  {
    QDir dir(fileString);
    if (dir.exists())
    {
      return dir.removeRecursively();
    }
    return true;
  }
  if (Command == "makeDir")
  {
    return vtksys::SystemTools::MakeDirectory(fileString.toUtf8().toStdString()).IsSuccess();
  }
  if (!this->Superclass::playEvent(Object, Command, Arguments, Error))
  {
    qCritical() << "Unknown pqFileDialog command: " << Object << " " << Command << " " << Arguments;
    Error = true;
  }
  return true;
}
