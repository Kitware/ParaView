/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqPropertyLinks_h
#define _pqPropertyLinks_h

#include "pqWidgetsExport.h"
#include <QObject>

class vtkObject;
class vtkSMProxy;
class vtkSMProperty;

/// provides direct links between Qt widgets and server manager properties
/// changing the value of a widget automatically updates the server manager
/// a change in the server manager automatically updates the widget
class PQWIDGETS_EXPORT pqPropertyLinks : public QObject
{
  Q_OBJECT
  
public:
  /// constructor
  pqPropertyLinks(QObject* p=0);
  /// destructor
  ~pqPropertyLinks();

  /// link a property
  void addPropertyLink(QObject* qObject, const char* qProperty, const char* signal,
                       vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=0);
  
  /// un-link a property
  void removePropertyLink(QObject* qObject, const char* qProperty, const char* signal,
                          vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=0);

  /// TODO: domain change events
  /// TODO: do domain changes possibly affect our links?

protected:
  
  class pqInternal;
  pqInternal* Internal;

};


/// pqPropertyLinks private helper class
class PQWIDGETS_EXPORT pqPropertyLinksConnection : public QObject
{
  Q_OBJECT
  friend class pqPropertyLinks;
public:
  pqPropertyLinksConnection(vtkSMProxy* proxy, vtkSMProperty* property, int idx,
                            QObject* qobject, const char* qproperty);
  pqPropertyLinksConnection(const pqPropertyLinksConnection& copy);
  ~pqPropertyLinksConnection();
  pqPropertyLinksConnection& operator=(const pqPropertyLinksConnection& copy);
  bool operator<(pqPropertyLinksConnection const& other) const;
private slots:
  void smLinkedPropertyChanged() const;
  void qtLinkedPropertyChanged() const;
private:
  class pqInternal;
  pqInternal* Internal;
};

#endif // !_pqPropertyLinks_h

