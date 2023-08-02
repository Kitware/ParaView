// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFileUtilitiesEventPlayer.h"

#include "pqCoreTestUtility.h"
#include <vtksys/SystemTools.hxx>

#include <QFile>
#include <QFileInfo>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqFileUtilitiesEventPlayer::pqFileUtilitiesEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
bool pqFileUtilitiesEventPlayer::playEvent(
  QObject*, const QString& command, const QString& args, bool& errorFlag)
{
  if (command == "file_exists")
  {
    const auto fname = pqCoreTestUtility::fixPath(args);
    const QFileInfo finfo(fname);
    if (!finfo.exists())
    {
      qCritical() << "Path not found: '" << fname << "'.";
      errorFlag = true;
      return true;
    }
    else if (finfo.exists() && !finfo.isFile())
    {
      qCritical() << "Path exists but is not a file: '" << fname << "'.";
      errorFlag = true;
      return true;
    }

    // all's well
    return true;
  }
  else if (command == "remove_file")
  {
    const auto fname = pqCoreTestUtility::fixPath(args);
    if (QFile::exists(fname))
    {
      if (!QFile::remove(fname))
      {
        qCritical() << "Failed to remove file: '" << fname << "'.";
        errorFlag = true;
      }
    }
    return true;
  }
  else if (command == "directory_exists")
  {
    const auto fname = pqCoreTestUtility::fixPath(args);
    const QFileInfo finfo(fname);
    if (!finfo.exists())
    {
      qCritical() << "Path not found: '" << fname << "'.";
      errorFlag = true;
      return true;
    }
    else if (finfo.exists() && !finfo.isDir())
    {
      qCritical() << "Path exists but is not a directory: '" << fname << "'.";
      errorFlag = true;
      return true;
    }

    // all's well
    return true;
  }
  else if (command == "remove_directory")
  {
    const auto fname = pqCoreTestUtility::fixPath(args);
    const QFileInfo finfo(fname);
    if (finfo.exists())
    {
      if (!vtksys::SystemTools::RemoveADirectory(qUtf8Printable(fname)))
      {
        qCritical() << "Failed to remove directory: " << fname;
        errorFlag = true;
      }
    }
    return true;
  }
  else if (command == "make_directory")
  {
    const auto fname = pqCoreTestUtility::fixPath(args);
    if (!vtksys::SystemTools::MakeDirectory(qUtf8Printable(fname)))
    {
      qCritical() << "Failed to make directory: " << fname;
      errorFlag = true;
    }
    return true;
  }
  else if (command == "copy_directory")
  {
    const auto fnames = pqCoreTestUtility::fixPath(args).split(";");
    if (fnames.size() != 2)
    {
      qCritical() << "Expecting 2 semi-colon separated arguments, got " << fnames.size() << ".";
      errorFlag = true;
    }
    else if (!vtksys::SystemTools::CopyADirectory(
               qUtf8Printable(fnames[0]), qUtf8Printable(fnames[1])))
    {
      qCritical() << "Failed to copy '" << fnames[0] << "' to '" << fnames[1] << "'.";
      errorFlag = true;
    }

    return true;
  }

  return false; // unhandled.
}
