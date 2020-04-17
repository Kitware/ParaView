/*=========================================================================

   Program: ParaView
   Module:    pqPropertyManager.h

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

#ifndef _pqPropertyManager_h
#define _pqPropertyManager_h

#include "pqCoreModule.h"
#include <QObject>
#include <QPointer>
#include <QVariant>

class vtkSMProxy;
class vtkSMProperty;
class pqPropertyLinks;

/**
* Manages links between Qt properties and unchecked proxy properties
* This is useful if more than one QWidget exposes a single proxy property
* In which case the server manager will not keep the widgets synchronized
* Also provides a mechanims for accepting or rejecting changes for unchecked
* properties
*/
class PQCORE_EXPORT pqPropertyManager : public QObject
{
  Q_OBJECT

public:
  /**
  * constructor
  */
  pqPropertyManager(QObject* p = 0);
  /**
  * destructor
  */
  ~pqPropertyManager() override;

  /**
  * register a QObject property to link with the server manager
  */
  void registerLink(QObject* qObject, const char* qProperty, const char* signal, vtkSMProxy* Proxy,
    vtkSMProperty* Property, int Index = -1);

  /**
  * unregister a QObject property to link with the server manager
  */
  void unregisterLink(QObject* qObject, const char* qProperty, const char* signal,
    vtkSMProxy* Proxy, vtkSMProperty* Property, int Index = -1);

  /**
  * returns whether there are modified properties to send to
  * the server manager
  */
  bool isModified() const;

  // Call this method to un-links all property links
  // maintained by this object.
  void removeAllLinks();

Q_SIGNALS:
  /**
  * Signal emitted when there are possible properties to send down to
  * the server manager
  */
  void modified();

  /**
  * Signal emitted when the user has accepted changes
  */
  void aboutToAccept();
  void accepted();
  /**
  * Signal emitted when the user has rejected changes
  */
  void rejected();

public Q_SLOTS:
  /**
  * accept property changes by pushing them all down to the server manager
  */
  void accept();
  /**
  * reject property changes and revert all QObject properties
  */
  void reject();
  /**
  * Called whenever a property changes from the GUI side
  */
  void propertyChanged();

protected:
  pqPropertyLinks* Links;
  bool Modified;
};
#endif // !_pqPropertyManager_h
