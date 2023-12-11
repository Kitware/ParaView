// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMySphereSource.h"

#include "vtkPVLogger.h"
#include <vtkObjectFactory.h>

#include <QByteArray>
#include <QFile>

vtkStandardNewMacro(vtkMySphereSource);

//----------------------------------------------------------------------------
vtkMySphereSource::vtkMySphereSource()
{
  // We need to manually init the resources.
  // https://doc.qt.io/qt-5/qdir.html#Q_INIT_RESOURCE
  // https://doc.qt.io/qt-5/resources.html#using-resources-in-a-library
  Q_INIT_RESOURCE(SphereResources);

  auto resources = QFile(":/myResource.txt");
  if (!resources.open(QIODevice::ReadOnly))
  {
    vtkLog(ERROR, "Fails to load resources.");
    return;
  }

  while (!resources.atEnd())
  {
    QByteArray line = resources.readLine();
    vtkLog(INFO, "read line: " << line.data());
  }
}

//----------------------------------------------------------------------------
vtkMySphereSource::~vtkMySphereSource() = default;

//----------------------------------------------------------------------------
void vtkMySphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
