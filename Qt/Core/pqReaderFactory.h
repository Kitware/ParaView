/*=========================================================================

   Program: ParaView
   Module:    pqReaderFactory.h

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
#ifndef __pqReaderFactory_h
#define __pqReaderFactory_h

#include "pqCoreExport.h"
#include <QObject>

class pqPipelineSource;
class pqReaderFactoryInternal;
class pqServer;
class vtkSMProxy;

// This class is a reader factory. The factory must be made file-type aware
// by using addFileType. Once initialized, one can use createReader()
// to create a reader that can read a particular file. 
class PQCORE_EXPORT pqReaderFactory : public QObject
{
public:
  pqReaderFactory(QObject* parent=NULL);
  virtual ~pqReaderFactory();

  // Register an extension (or extensions) with a particular reader proxy 
  // identified by
  // the \c xmlgroup and \c xmlname. Same extension can be associated with
  // more than one reader, however, in that case, the reader(s) must support
  // \c CanReadFile() for this work correctly.
  void addFileType(const QString& description, const QString& extension,
    const QString& xmlgroup, const QString& xmlname);

  void addFileType(const QString& description, const QList<QString>& extensions,
    const QString& xmlgroup, const QString& xmlname);

  // An overload of addFileType where one can specify the prototype of the
  // reader proxy.
  void addFileType(const QString& description, const QString& extension,
    vtkSMProxy* prototype);
  void addFileType(const QString& description, const QList<QString>& extensions,
    vtkSMProxy* prototype);

  // Create a reader that can read the given file present on the given server.
  // File types must be registered before a file of the given type can be read.
  // This method creates and registers the reader proxy that can read
  pqPipelineSource* createReader(const QString& filename, pqServer* server);

  // Returns a list of file types suitable for use with file dialog.
  // \c server is required to ensure that only those readers that can
  // be instantiated on the server will be considered.
  QString getSupportedFileTypes(pqServer* server);

  // Loads file type definitions from the xml file.
  // Format of this xml is:
  // \verbatim
  // <ParaViewReaders>
  //    <Reader name="[reader name should match the xmlname of the proxy]" 
  //      extensions="[space separated extensions supported, dont include ." 
  //      file_description="[short description]"
  //      group=[optional: server manager group under which the reader definition 
  //            can be found.]" >
  //    </Reader>
  //    ...
  // </ParaViewReaders>
  // \endverbatim
  // By default, the reader is searched for under the \c sources group.
  void loadFileTypes(const QString& xmlfilename);
protected:
  bool checkIfFileIsReadable(const QString& name, pqServer*);

private:
  pqReaderFactoryInternal* Internal;
};


#endif

