/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkManagerModel.h

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
#ifndef __pqLookmarkManagerModel_h
#define __pqLookmarkManagerModel_h

#include "pqComponentsExport.h"
#include <QObject>

class pqView;
class pqServer;
class pqLookmarkManagerModelInternal;
class pqLookmarkManagerModelItem;
class pqLookmarkModel;
class QImage;
class pqPipelineSource;

/// \class pqLookmarkManagerModel
/// \brief
///   The pqLookmarkManagerModel class performs the book-keeping and storage of lookmarks. Any model of lookmarks should be built on top of this representation.
///
/// Lookmarks can be imported or exported in Lookmark Definition Files. This is an XML representation of the set of lookmark(s). Here is the format:
///   <LookmarkDefinitionFile>
///     <LookmarkDefinition name="My Lookmark" Comments="Here are a few thoughts..." RestoreData="1" RestoreCamera="1">
///         <Icon value="KDJFLSKDJFLJDLSKFJLDSKJFLSDJFLSDKJFLKDSJFLSDKFLSL.../>
///         <Pipeline value="KDJFLSKDJFLJDLSKFJLDSKJFLSDJFLSDKJFLKDSJFLSDKFLSL.../>
///         <ServerManagerState>
///           ....
///         </ServerManagerState>
///     </LookmarkDefinition>
///   ....
///   </LookmarkDefinitionFile>
///
/// This same format is used to store lookmarks between sessions of ParaView in the application settings. 
///
/// TO DO: Add support for hierarchicaly storage of lookmarks.

class PQCOMPONENTS_EXPORT pqLookmarkManagerModel : public QObject
{
  Q_OBJECT
public:
  pqLookmarkManagerModel(QObject* parent=NULL);
  virtual ~pqLookmarkManagerModel();

  // Return the number of lookmarks in the list
  int getNumberOfLookmarks();

  // Get the lookmark at the given index in the list
  pqLookmarkModel* getLookmark(int index) const;
  // Get the lookmark with the given name (all lookmarks have unique names)
  pqLookmarkModel* getLookmark(const QString &name) const;
  // Get a list of all lookmarks
  QList<pqLookmarkModel*> getAllLookmarks() const;
  // Get an XML representation of the collection of lookmarks
  QString getAllLookmarksSerialized() const;
  // Get an XML representation of the given set of lookmarks
  QString getLookmarksSerialized(const QList<pqLookmarkModel*> &lookmarks) const;
 
public slots:

  // Add the given lookmark to collection
  void addLookmark(pqLookmarkModel *lookmark);

  // Remove the lookmark with the given name (all lookmarks have unique names)
  void removeLookmark(const QString &name);
  // Remove the lookmark from the collection
  void removeLookmark(pqLookmarkModel *lookmark);
  // Remove the lookmarks in the given list from the collection
  void removeLookmarks(const QList<pqLookmarkModel*> &lookmarks);
  // Remove all stored lookmarks
  void removeAllLookmarks();

  // Parse the given XML-formatted files into lookmark objects and add them to the collection
  void importLookmarksFromFiles(const QStringList &files);
  // Parse the XML-formatted string in pqSettings into lookmark objects and add them to the collection.
  // This is done on startup to populate the collection from the lookmarks in the last session.
  void importLookmarksFromSettings();

  // Save the entire collection to the given files as XML
  void exportAllLookmarksToFiles(const QStringList &files);
  // Save the lookmarks in the given list to the given files as XML
  void exportLookmarksToFiles(const QList<pqLookmarkModel*> &lookmarks, 
    const QStringList &files);
  // Save the entire collection of lookmarks to pqSettings. This is done 
  // automatically when the application closes
  void exportAllLookmarksToSettings();

  // Load the state of the lookmark with the given name, on the given server, 
  // in the given view
  void loadLookmark(pqServer *server, pqView* dest, 
    QList<pqPipelineSource*> *sources, const QString &name);

  // Load the state of the given lookmark, on the given server, 
  // in the given view
  void loadLookmark(pqServer *server, pqView* dest, 
    QList<pqPipelineSource*> *sources, pqLookmarkModel *lmk);

signals:

  void lookmarkLoaded(pqLookmarkModel*);
  // Some views (the toolbar) only need the name and icon of the lookmark rather than the lookmark object itself
  void lookmarkAdded(const QString &name, const QImage &icon);
  void lookmarkAdded(pqLookmarkModel*);
  void lookmarkRemoved(const QString &name);
  void lookmarkModified(pqLookmarkModel*);
  void lookmarkNameChanged(const QString &oldName,const QString &newName);

protected:
  QString getUnusedLookmarkName(const QString &name);

private:
  pqLookmarkManagerModelInternal* Internal;
};

#endif

