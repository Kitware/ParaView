/*=========================================================================

   Program: ParaView
   Module:    pqEventSource.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqEventSource_h
#define _pqEventSource_h

#include "QtTestingExport.h"
#include <QObject>
class QString;

/// Abstract interface for objects that can supply high-level testing events
class QTTESTING_EXPORT pqEventSource : public QObject
{
  Q_OBJECT
public:
  pqEventSource(QObject* p) : QObject(p) {}
  virtual ~pqEventSource() {}

  enum { SUCCESS, FAILURE, DONE };

  /** Retrieves the next available event.  
    Returns SUCCESS if an event was returned and can be processed,
    FAILURE if there was a problem,
    DONE, if there are no more events. */
  virtual int getNextEvent(
    QString& object,
    QString& command,
    QString& arguments) = 0;

  /** Set the filename for contents.
      Returns true for valid file, false for invalid file */
  virtual void setContent(const QString& filename) = 0;

  /** tell the source to stop feeding in events */
  virtual void stop() {}

};

#endif // !_pqEventSource_h

