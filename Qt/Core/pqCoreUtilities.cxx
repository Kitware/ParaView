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

#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QStringList>

#include "pqApplicationCore.h"
#include "pqSettings.h"
#include "vtkNumberToString.h"
#include "vtkObject.h"
#include "vtkWeakPointer.h"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <cstdlib>
#include <sstream>

QPointer<QWidget> pqCoreUtilities::MainWidget = 0;

//-----------------------------------------------------------------------------
QWidget* pqCoreUtilities::findMainWindow()
{
  foreach (QWidget* widget, QApplication::topLevelWidgets())
  {
    if (widget->isWindow() && widget->isVisible() && qobject_cast<QMainWindow*>(widget))
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
  pqSettings* settings = pqApplicationCore::instance()->settings();
  return QFileInfo(settings->fileName()).path();
}

//-----------------------------------------------------------------------------
QString pqCoreUtilities::getParaViewApplicationDirectory()
{
  return QApplication::applicationDirPath();
}

//-----------------------------------------------------------------------------
QStringList pqCoreUtilities::findParaviewPaths(
  QString directoryOrFileName, bool lookupInAppDir, bool lookupInUserDir)
{
  QStringList allPossibleDirs;
  if (lookupInAppDir)
  {
    allPossibleDirs.push_back(
      getParaViewApplicationDirectory() + QDir::separator() + directoryOrFileName);
    allPossibleDirs.push_back(getParaViewApplicationDirectory() + "/../" + directoryOrFileName);
    // Mac specific begin
    allPossibleDirs.push_back(
      getParaViewApplicationDirectory() + "/../Support/" + directoryOrFileName);
    allPossibleDirs.push_back(
      getParaViewApplicationDirectory() + "/../../../Support/" + directoryOrFileName);
    // This one's for when running from the build directory.
    allPossibleDirs.push_back(
      getParaViewApplicationDirectory() + "/../../../" + directoryOrFileName);
    // Mac specific end
  }

  if (lookupInUserDir)
  {
    allPossibleDirs.push_back(getParaViewUserDirectory() + QDir::separator() + directoryOrFileName);
  }

  // Filter with only existing ones
  QStringList existingDirs;
  foreach (QString path, allPossibleDirs)
  {
    if (QFile::exists(path))
      existingDirs.push_back(path);
  }

  return existingDirs;
}

//-----------------------------------------------------------------------------
QString pqCoreUtilities::getNoneExistingFileName(QString expectedFilePath)
{
  QDir dir = QFileInfo(expectedFilePath).absoluteDir();
  QString baseName = QFileInfo(expectedFilePath).fileName();

  // Extract extension
  QString extension;
  if (baseName.lastIndexOf(".") != -1)
  {
    extension = baseName;
    extension.remove(0, baseName.lastIndexOf("."));
    baseName.chop(extension.size());
  }

  QString fileName = baseName + extension;
  int index = 1;
  while (dir.exists(fileName))
  {
    fileName = baseName;
    fileName.append("-").append(QString::number(index)).append(extension);
    index++;
  }

  return dir.absolutePath() + QDir::separator() + fileName;
}

//-----------------------------------------------------------------------------
class pqCoreUtilitiesEventHelper::pqInternal
{
public:
  vtkWeakPointer<vtkObject> EventInvoker;
  unsigned long EventID;
  pqInternal()
    : EventID(0)
  {
  }

  ~pqInternal()
  {
    if (this->EventInvoker && this->EventID > 0)
    {
      this->EventInvoker->RemoveObserver(this->EventID);
    }
  }
};

//-----------------------------------------------------------------------------
pqCoreUtilitiesEventHelper::pqCoreUtilitiesEventHelper(QObject* object)
  : Superclass(object)
  , Interal(new pqCoreUtilitiesEventHelper::pqInternal())
{
}

//-----------------------------------------------------------------------------
pqCoreUtilitiesEventHelper::~pqCoreUtilitiesEventHelper()
{
  delete this->Interal;
}

//-----------------------------------------------------------------------------
void pqCoreUtilitiesEventHelper::executeEvent(vtkObject* obj, unsigned long eventid, void* calldata)
{
  emit this->eventInvoked(obj, eventid, calldata);
}

//-----------------------------------------------------------------------------
unsigned long pqCoreUtilities::connect(vtkObject* vtk_object, int vtk_event_id, QObject* qobject,
  const char* signal_or_slot, Qt::ConnectionType type /* = Qt::AutoConnection*/)
{
  assert(vtk_object != NULL);
  assert(qobject != NULL);
  assert(signal_or_slot != NULL);
  if (vtk_object == NULL || qobject == NULL || signal_or_slot == NULL)
  {
    // qCritical is Qt's 'print error message' stream
    qCritical() << "Error: Cannot connect to or from NULL.";
    return 0;
  }

  pqCoreUtilitiesEventHelper* helper = new pqCoreUtilitiesEventHelper(qobject);
  unsigned long eventid =
    vtk_object->AddObserver(vtk_event_id, helper, &pqCoreUtilitiesEventHelper::executeEvent);
  helper->Interal->EventID = eventid;
  helper->Interal->EventInvoker = vtk_object;

  QObject::connect(
    helper, SIGNAL(eventInvoked(vtkObject*, unsigned long, void*)), qobject, signal_or_slot, type);

  // * When qobject is deleted, helper is deleted. pqCoreUtilitiesEventHelper in
  // its destructor ensures that the observer is removed from the vtk_object if
  // it exists.
  // * When VTK-object is deleted, it removes the observer, but cannot delete
  // helper. Since pqCoreUtilitiesEventHelper::Interal keeps a weak-pointer to
  // the vtk_object, that gets cleared. So eventually when qobject is destroyed,
  // the pqCoreUtilitiesEventHelper is deleted, but since the vtk_object is
  // already deleted, it doesn't do anything special.
  return eventid;
}

//-----------------------------------------------------------------------------
bool pqCoreUtilities::promptUser(const QString& settingsKey, QMessageBox::Icon icon,
  const QString& title, const QString& message, QMessageBox::StandardButtons buttons,
  QWidget* parentWdg)
{
  if (vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") != NULL)
  {
    return true;
  }
  parentWdg = parentWdg ? parentWdg : pqCoreUtilities::mainWidget();

  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains(settingsKey))
  {
    return true;
  }

  QMessageBox mbox(icon, title, message, buttons, parentWdg);
  mbox.setObjectName("CoreUtilitiesPromptUser");

  // Add a "Yes, and don't ask" button.
  QAbstractButton* remember = mbox.button(QMessageBox::Save);
  QAbstractButton* yesButton = mbox.button(QMessageBox::Yes);
  QAbstractButton* okButton = mbox.button(QMessageBox::Ok);
  if (yesButton && remember)
  {
    remember->setText("Yes, and don't ask again");
    remember->setObjectName("YesAndSave");
    remember->setIcon(mbox.button(QMessageBox::Yes)->icon());
  }
  else if (okButton && remember)
  {
    remember->setText("OK, and don't ask again");
    remember->setObjectName("OkAndSave");
    remember->setIcon(mbox.button(QMessageBox::Ok)->icon());
  }
  mbox.exec();

  switch (mbox.standardButton(mbox.clickedButton()))
  {
    case QMessageBox::Save:
      settings->setValue(settingsKey, true);
      return true;
    case QMessageBox::Yes:
      return true;
    case QMessageBox::No:
    default:
      return false;
  }
}

//-----------------------------------------------------------------------------
QString pqCoreUtilities::number(double value)
{
  std::ostringstream str;
  str << vtkNumberToString()(value);
  return QString::fromLocal8Bit(str.str().c_str());
}
