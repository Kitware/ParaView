/*=========================================================================

   Program: ParaView
   Module:    pqCoreUtilities.cxx

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
#include "pqCoreUtilities.h"

#include <QMainWindow>
#include <QApplication>
#include <QDir>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QFileInfo>

QPointer<QWidget> pqCoreUtilities::MainWidget = 0;

//-----------------------------------------------------------------------------
QWidget* pqCoreUtilities::findMainWindow()
{
  foreach (QWidget* widget, QApplication::topLevelWidgets())
    {
    if (widget->isWindow() && widget->isVisible() &&
      qobject_cast<QMainWindow*>(widget))
      {
      return widget;
      }
    }

  // Find any window (even if not visible).
  foreach (QWidget* widget, QApplication::topLevelWidgets())
    {
    if (widget->isWindow() && qobject_cast<QMainWindow*>(widget))
      {
      return widget;
      }
    }

  return NULL;
}

//-----------------------------------------------------------------------------
QString pqCoreUtilities::getParaViewUserDirectory()
  {
  QString settingsRoot;
#if defined(Q_OS_WIN)
  settingsRoot = QString::fromLocal8Bit(getenv("APPDATA"));
#else
  settingsRoot = QString::fromLocal8Bit(getenv("HOME")) +
                 QDir::separator() + QString::fromLocal8Bit(".config");
#endif
  QString settingsPath = QString("%2%1%3");
  settingsPath = settingsPath.arg(QDir::separator());
  settingsPath = settingsPath.arg(settingsRoot);
  settingsPath = settingsPath.arg(QApplication::organizationName());
  return settingsPath;
  }

//-----------------------------------------------------------------------------
QString pqCoreUtilities::getParaViewApplicationDirectory()
  {
  return QApplication::applicationDirPath();
  }

//-----------------------------------------------------------------------------
QStringList pqCoreUtilities::findParaviewPaths(QString directoryOrFileName,
                                               bool lookupInAppDir,
                                               bool lookupInUserDir)
  {
  QStringList allPossibleDirs;
  if(lookupInAppDir)
    {
    allPossibleDirs.push_back( getParaViewApplicationDirectory()
                               + QDir::separator() + directoryOrFileName);
    allPossibleDirs.push_back( getParaViewApplicationDirectory()
                               + "/../" + directoryOrFileName);
    // Mac specific begin
    allPossibleDirs.push_back( getParaViewApplicationDirectory()
                               + "/../Support/" + directoryOrFileName);
    allPossibleDirs.push_back( getParaViewApplicationDirectory()
                               + "/../../../Support/" + directoryOrFileName);
    // Mac specific end
    }

  if(lookupInUserDir)
    {
    allPossibleDirs.push_back( getParaViewUserDirectory() + QDir::separator()
                               + directoryOrFileName);
    }

  // Filter with only existing ones
  QStringList existingDirs;
  foreach(QString path, allPossibleDirs)
    {
    if(QFile::exists(path))
      existingDirs.push_back(path);
    }

  return existingDirs;
  }

//-----------------------------------------------------------------------------
QString pqCoreUtilities::getNoneExistingFileName(QString expectedFilePath)
  {
  QDir dir  = QFileInfo(expectedFilePath).absoluteDir();
  QString baseName = QFileInfo(expectedFilePath).fileName();

  // Extract extension
  QString extension;
  if(baseName.lastIndexOf(".") != -1)
    {
    extension = baseName;
    extension.remove(0, baseName.lastIndexOf("."));
    baseName.chop(extension.size());
    }

  QString fileName = baseName + extension;
  int index = 1;
  while(dir.exists(fileName))
    {
    fileName = baseName;
    fileName.append("-").append(QString::number(index)).append(extension);
    index++;
    }

  return dir.absolutePath() + QDir::separator() + fileName;
  }
//-----------------------------------------------------------------------------
