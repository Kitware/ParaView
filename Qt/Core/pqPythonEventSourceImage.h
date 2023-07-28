// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPythonEventSourceImage_h
#define pqPythonEventSourceImage_h

#include "pqCoreModule.h"
#include "pqPythonEventSource.h"

/** Python event source with Image comparison capabilities

 import QtTestingImage
 QtTestingImage.compareImage('widgetName', 'baseline', threshold, 'tempDir')



 */
class vtkImageData;

class PQCORE_EXPORT pqPythonEventSourceImage : public pqPythonEventSource
{
  Q_OBJECT
public:
  pqPythonEventSourceImage(QObject* p = 0);
  ~pqPythonEventSourceImage();

protected:
  virtual void run();

protected Q_SLOTS:
  void doComparison();
};

#endif // !pqPythonEventSourceImage_h
