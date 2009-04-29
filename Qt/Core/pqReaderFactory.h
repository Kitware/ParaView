/*=========================================================================

   Program: ParaView
   Module:    pqReaderFactory.h

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
#ifndef __pqReaderFactory_h
#define __pqReaderFactory_h

#include "pqCoreExport.h"
#include <QObject>

class pqPipelineSource;
class pqReaderFactoryInternal;
class pqServer;
class vtkSMProxy;
class vtkSMProperty;

/// This class is a reader factory. The factory must be made file-type aware
/// by using addFileType. Once initialized, one can use createReader()
/// to create a reader that can read a particular file. 
/// Applications that want to use the reader factory for creating readers,
/// must instantiate their own reader factory, initialize it and then
/// use it to create readers. pqApplicationCore/pqObjectBuilder does not require 
/// that a reader factory is used at all.
class PQCORE_EXPORT pqReaderFactory : public QObject
{
  Q_OBJECT
public:
  pqReaderFactory(QObject* parent=NULL);
  virtual ~pqReaderFactory();

  /// Register an extension (or extensions) with a particular reader proxy 
  /// identified by
  /// the \c xmlgroup and \c xmlname. Same extension can be associated with
  /// more than one reader, however, in that case, the reader(s) must support
  /// \c CanReadFile() for this work correctly.
  void addFileType(const QString& description, const QString& extension,
    const QString& xmlgroup, const QString& xmlname);

  void addFileType(const QString& description, const QList<QString>& extensions,
    const QString& xmlgroup, const QString& xmlname);

  /// An overload of addFileType where one can specify the prototype of the
  /// reader proxy.
  void addFileType(const QString& description, const QString& extension,
    vtkSMProxy* prototype);
  void addFileType(const QString& description, const QList<QString>& extensions,
    vtkSMProxy* prototype);

  /// Create a reader given by name on the given server to read the given
  /// file(s).  File types must be registered before a file of the given
  /// type can be read.  This method creates and registers the reader proxy
  /// that can read
  pqPipelineSource* createReader(const QStringList& files,
    const QString& readerName, pqServer* server);

  /// Returns a list of file types suitable for use with file dialog.
  /// \c server is required to ensure that only those readers that can
  /// be instantiated on the server will be considered.
  QString getSupportedFileTypes(pqServer* server);
  
  /// Returns a list of the supported readers on a server.
  /// \c server is required to ensure that only those readers that can
  /// be instantiated on the server will be considered.
  QStringList getSupportedReaders(pqServer* server);

  /// Same as getSupportedReaders but further constrains the list to contain
  /// only those readers that report they can read the file pointed to by
  /// \c filename (or perhaps do not report anything)
  QStringList getSupportedReadersForFile(pqServer* server,
                                         const QString &filename);

  /// Returns a short description of the reader.
  QString getReaderDescription(const QString& readerName);

  /// Returns the list of extensions for a reader
  QString getExtensionTypeString(pqPipelineSource* reader);

  /// Return the reader type for a file
  QString getReaderType(const QString& filename, pqServer*);

  bool checkIfFileIsReadable(const QString& name, pqServer*);

public slots: 
  
  /// loads file types from the Qt resource directory
  /// ":/ParaViewResources/"
  void loadFileTypes();
 
protected: 
  
  /// Loads file type definitions from the xml file.
  /// Format of this xml is:
  /// \verbatim
  /// <ParaViewReaders>
  ///    <Reader name="[reader name should match the xmlname of the proxy]" 
  ///      extensions="[space separated extensions supported, dont include ." 
  ///      file_description="[short description]"
  ///      group=[optional: server manager group under which the reader definition 
  ///            can be found.]" >
  ///    </Reader>
  ///    ...
  /// </ParaViewReaders>
  /// \endverbatim
  /// By default, the reader is searched for under the \c sources group.
  void loadFileTypes(const QString& xmlfilename);

private:
  pqReaderFactoryInternal* Internal;
};


#endif

