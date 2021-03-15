/*=========================================================================

   Program: ParaView
   Module:  pqFileUtilitiesEventPlayer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
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
