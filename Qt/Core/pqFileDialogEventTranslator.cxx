// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFileDialogEventTranslator.h"
#include "pqCoreTestUtility.h"

#include <pqFileDialog.h>

#include <QDir>
#include <QEvent>
#include <QtDebug>

pqFileDialogEventTranslator::pqFileDialogEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

bool pqFileDialogEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& Error)
{
  // Capture input for pqFileDialog and all its children ...
  pqFileDialog* object = nullptr;
  for (QObject* o = Object; o; o = o->parent())
  {
    object = qobject_cast<pqFileDialog*>(o);
    if (object)
      break;
  }

  if (!object)
    return false;

  if (Event->type() == QEvent::FocusIn && !this->CurrentObject)
  {
    this->CurrentObject = object;
    connect(
      object, SIGNAL(fileAccepted(const QString&)), this, SLOT(onFilesSelected(const QString&)));
    connect(object, SIGNAL(rejected()), this, SLOT(onCancelled()));
    return true;
  }

  return this->Superclass::translateEvent(Object, Event, Error);
}

void pqFileDialogEventTranslator::onFilesSelected(const QString& file)
{
  QString cleanedFile = QDir::cleanPath(file);
  QString data_directory = pqCoreTestUtility::DataRoot();
  if (data_directory.isEmpty())
  {
    qWarning()
      << "You must set the PARAVIEW_DATA_ROOT environment variable to play-back file selections.";
  }
  else if (cleanedFile.startsWith(data_directory, Qt::CaseInsensitive))
  {
    cleanedFile.replace(0, data_directory.size(), "$PARAVIEW_DATA_ROOT");
  }
  else
  {
    qWarning()
      << "You must choose a file under the PARAVIEW_DATA_ROOT directory to record file selections.";
  }

  Q_EMIT recordEvent(this->CurrentObject, "filesSelected", cleanedFile);
}

void pqFileDialogEventTranslator::onCancelled()
{
  Q_EMIT recordEvent(this->CurrentObject, "cancelled", "");
}
