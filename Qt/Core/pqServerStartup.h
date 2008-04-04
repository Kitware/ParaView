/*=========================================================================

   Program: ParaView
   Module:    pqServerStartup.h

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

#ifndef _pqServerStartup_h
#define _pqServerStartup_h

#include "pqServerResource.h"
#include "pqCoreExport.h"

#include <QObject>
#include <QMap>
class vtkPVXMLElement;

/////////////////////////////////////////////////////////////////////////////
// pqServerStartup

/// Abstract interface for an object that can start a remote server
class PQCORE_EXPORT pqServerStartup : public QObject
{
  Q_OBJECT
public:
  pqServerStartup(bool save, QObject* p=NULL)
    : QObject(p), ShouldSave(save) {}
  virtual ~pqServerStartup() {}

  /// Returns the name of this startup
  virtual const QString getName() = 0;
  /// Returns the server for this startup
  virtual const pqServerResource getServer() = 0;
  /// Returns an XML description of the configuration for this startup
  virtual vtkPVXMLElement* getConfiguration() = 0;
  /// Returns whether the startup can be saved
  virtual bool shouldSave() { return ShouldSave; }
  
  /// Defines a generic collection of name-value-pair "options" that will be
  /// set by the user prior to server startup
  typedef QMap<QString, QString> OptionsT;
    
  /** Begins (asynchronous) execution of the startup procedure.
  Callers should connect to success() and failure() signals to receive 
  notification that the startup procedure has been completed. */
  virtual void execute(const OptionsT& options) = 0;

signals:
  void succeeded();
  void failed();

protected:
  bool ShouldSave;
  
private:
  pqServerStartup(const pqServerStartup&);  // not implemented
  pqServerStartup& operator=(const pqServerStartup&);  //  not implemented
};

#endif // !_pqServerStartup_h

