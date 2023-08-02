// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDesktopServicesReaction.h"

#include "pqCoreUtilities.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QtDebug>

#include <iostream>
//-----------------------------------------------------------------------------
pqDesktopServicesReaction::pqDesktopServicesReaction(const QUrl& url, QAction* parentObject)
  : Superclass(parentObject)
  , URL(url)
{
  if (parentObject)
  {
    parentObject->setStatusTip(url.toString());
  }
}

//-----------------------------------------------------------------------------
pqDesktopServicesReaction::~pqDesktopServicesReaction() = default;

//-----------------------------------------------------------------------------
bool pqDesktopServicesReaction::openUrl(const QUrl& url)
{
  if (url.isLocalFile() && !QFileInfo(url.toLocalFile()).exists())
  {
    QString filename = QFileInfo(url.toLocalFile()).absoluteFilePath();
    QString msg =
      tr("The requested file is not available in your installation. "
         "You can manually obtain and place the file (or ask your administrators) at the "
         "following location for this to work.\n\n'%1'")
        .arg(filename);
    // dump to cout for easy copy/paste.
    std::cout << msg.toUtf8().data() << std::endl;
    QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Missing file"), msg, QMessageBox::Ok);
    return false;
  }
  if (!QDesktopServices::openUrl(url))
  {
    qCritical() << "Failed to open '" << url << "'";
    return false;
  }
  return true;
}
