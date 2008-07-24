/*=========================================================================

   Program: ParaView
   Module:    pqWriterFactory.h

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
#ifndef __pqWriterFactory_h
#define __pqWriterFactory_h

#include "pqCoreExport.h"
#include <QObject>

class vtkSMProxy;
class pqWriterFactoryInternal;
class pqOutputPort;


// This is a writer factory. The factory must be made file-type aware
// by using addFileType. Once initialized, one can use createWriter()
// to create a writer that can write the output of a particular source
// to a given file.
class  PQCORE_EXPORT pqWriterFactory : public QObject
{
  Q_OBJECT
public:
  pqWriterFactory(QObject* parent=NULL);
  virtual ~pqWriterFactory();

  // Register an extension (or extensions) with a particular writer proxy 
  // identified by
  // the \c xmlgroup and \c xmlname. Same extension can be associated with
  // more than one writer. A writer is choosen based on extension match and
  // data type match. If more than one writer fits the criteria, the first
  // one gets the priority.
  void addFileType(const QString& description, const QString& extension,
    const QString& xmlgroup, const QString& xmlname);
  void addFileType(const QString& description, const QList<QString>& extensions,
    const QString& xmlgroup, const QString& xmlname);

  // An overload of addFileType where one can specify the prototype of the
  // writer proxy.
  void addFileType(const QString& description, const QString& extension,
    vtkSMProxy* prototype);
  void addFileType(const QString& description, const QList<QString>& extensions,
    vtkSMProxy* prototype);

  // Creates a write that can write the output of the \c toWrite source to
  // \c filename file. This allocates a new writer proxy and the caller
  // must call Delete() on the returned proxy.
  vtkSMProxy* newWriter(const QString& filename, pqOutputPort* toWrite);

  // Returns a file type filtering string suitable for file dialogs. 
  // Returns only those file formats that can be written using the 
  // output of given and the number of partitions on the data server
  // on which the source exists.
  QString getSupportedFileTypes(pqOutputPort* toWrite);

public slots:
  // load file types from ":/ParaViewResources/"
  void loadFileTypes();
  
protected: 
  // Initializes the factory from XML.
  void loadFileTypes(const QString& xmlfilename);

private:
  pqWriterFactoryInternal* Internal;
};


#endif

