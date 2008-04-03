/*=========================================================================

   Program: ParaView
   Module:    pqLookupTableManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqLookupTableManager_h
#define __pqLookupTableManager_h

#include <QObject>
#include "pqCoreExport.h"

class pqProxy;
class pqScalarsToColors;
class pqServer;

/// pqLookupTableManager is the manager that manages color lookup objects.
/// This is an abstract class that defines the API for any LUT manager.
/// subclasses are free to implement their own policy which can be specific
/// to the application.
class PQCORE_EXPORT pqLookupTableManager : public QObject
{
  Q_OBJECT
public:
  pqLookupTableManager(QObject* parent=NULL);
  virtual ~pqLookupTableManager();

  /// Get a LookupTable for the array with name \c arrayname 
  /// and component. component = -1 represents magnitude. Subclasses
  /// can implemenent their own policy for managing lookup tables.
  virtual pqScalarsToColors* getLookupTable(pqServer* server, 
    const QString& arrayname, int number_of_components, int component) = 0;

  /// Saves the state of the lut so that the next time a new LUT is created, it
  /// will have the same state as this one.
  virtual void saveAsDefault(pqScalarsToColors*)=0;
public slots:
  /// Called to update scalar ranges of all lookup tables.
  virtual void updateLookupTableScalarRanges()=0;

private slots:
  /// Called when any proxy is added. Subclasses can override
  /// onAddLookupTable() which is called by this method when it is
  /// ascertained that the proxy is a lookup table.
  void onAddProxy(pqProxy* proxy);

  /// Called when any proxy is removed. Subclasses can
  /// override onRemoveLookupTable which is called by this method
  /// with the proxy removed is a lookuptable.
  void onRemoveProxy(pqProxy* proxy);

protected:
  /// Called when a LUT is added.
  virtual void onAddLookupTable(pqScalarsToColors* lut) = 0;

  /// Called when a LUT is removed.
  virtual void onRemoveLookupTable(pqScalarsToColors* lut) = 0;

};


#endif

