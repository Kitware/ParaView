/*=========================================================================

   Program: ParaView
   Module:    pqLoadDataReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqLoadDataReaction_h
#define pqLoadDataReaction_h

#include "pqReaction.h"
#include <QList>

class QStringList;
class pqPipelineSource;
class pqServer;
class vtkSMReaderFactory;

/**
* @ingroup Reactions
* Reaction for open data files.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqLoadDataReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be nullptr.
  */
  pqLoadDataReaction(QAction* parent);

  /**
  * Loads multiple data files. Uses reader factory to determine what reader are
  * supported. If a file requires user input the reader of choice, it will use
  * that reader for all other files of that type.
  * Returns the reader is creation successful, otherwise returns
  * nullptr.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  *
  * If `readergroup` and `readername` are non empty, then they are assumed to be
  * the type of the reader to use and reader factory is not used.
  *
  * If `server` is nullptr, active server is used.
  */
  static pqPipelineSource* loadData(const QList<QStringList>& files,
    const QString& readergroup = QString(), const QString& readername = QString(),
    pqServer* server = nullptr);

  /**
  * Loads data files. Uses reader factory to determine what reader are
  * supported. Returns the reader is creation successful, otherwise returns
  * nullptr.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  *
  * If `readergroup` and `readername` are non empty, then they are assumed to be
  * the type of the reader to use and reader factory is not used.
  *
  * If `server` is nullptr, active server is used.
  */
  static pqPipelineSource* loadData(const QStringList& files,
    const QString& readergroup = QString(), const QString& readername = QString(),
    pqServer* server = nullptr);

  static QList<pqPipelineSource*> loadData();

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;

Q_SIGNALS:
  /**
  * Fired when a dataset is loaded by this reaction.
  */
  void loadedData(pqPipelineSource*);

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override
  {
    QList<pqPipelineSource*> sources = pqLoadDataReaction::loadData();
    pqPipelineSource* source;
    foreach (source, sources)
    {
      Q_EMIT this->loadedData(source);
    }
  }

  static bool TestFileReadability(
    const QString& file, pqServer* server, vtkSMReaderFactory* factory);

  static bool DetermineFileReader(const QString& filename, pqServer* server,
    vtkSMReaderFactory* factory, QPair<QString, QString>& readerInfo);

  static pqPipelineSource* LoadFile(
    const QStringList& files, pqServer* server, const QPair<QString, QString>& readerInfo);

private:
  Q_DISABLE_COPY(pqLoadDataReaction)
};

#endif
