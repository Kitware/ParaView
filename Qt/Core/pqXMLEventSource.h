// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqXMLEventSource_h
#define pqXMLEventSource_h

#include "pqCoreModule.h"
#include "pqEventSource.h"

class QString;

/** Concrete implementation of pqEventSource that retrieves events recorded
by pqXMLEventObserver */
class PQCORE_EXPORT pqXMLEventSource : public pqEventSource
{
  Q_OBJECT
public:
  pqXMLEventSource(QObject* p = nullptr);
  ~pqXMLEventSource() override;

  void setContent(const QString& path) override;

  /**
   * Get the next event from the event source
   */
  int getNextEvent(QString& object, QString& command, QString& arguments, int& eventType) override;

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !pqXMLEventSource_h
