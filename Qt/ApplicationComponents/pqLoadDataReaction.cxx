/*=========================================================================

   Program: ParaView
   Module:    pqLoadDataReaction.cxx

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
#include "pqLoadDataReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqSelectReaderDialog.h"
#include "pqServer.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqUndoStack.h"
#include "vtkFileSequenceParser.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMSettingsProxy.h"
#include "vtkStringList.h"
#include <vtksys/SystemTools.hxx>

#include <QDebug>
#include <QInputDialog>
#include <QMap>

#include <cassert>

//-----------------------------------------------------------------------------
pqLoadDataReaction::pqLoadDataReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqLoadDataReaction::updateEnableState()
{
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  // TODO: also is there's a pending accept.
  bool enable_state = (activeObjects.activeServer() != nullptr);
  this->parentAction()->setEnabled(enable_state);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqLoadDataReaction::loadData()
{
  ReaderSet readerSet;
  return pqLoadDataReaction::loadData(readerSet);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqLoadDataReaction::loadData(const ReaderSet& readerSet)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();

  std::vector<FileTypeDetailed> filtersDetailed =
    readerFactory->GetSupportedFileTypesDetailed(server->session());
  QString filtersString;

  // When using a readerSet, rewrite the Supported Types extensions to only list wanted
  // supported files
  if (!readerSet.isEmpty())
  {
    FileTypeDetailed* supportedTypesDetailed = nullptr;
    std::vector<std::string> supportedTypesPattern;
    for (auto& filterDetailed : filtersDetailed)
    {
      ReaderPair readerPair(
        QString::fromStdString(filterDetailed.Group), QString::fromStdString(filterDetailed.Name));
      if (filterDetailed.Description == vtkSMReaderFactory::SUPPORTED_TYPES_DESCRIPTION)
      {
        supportedTypesDetailed = &filterDetailed;
      }
      else if (readerSet.contains(readerPair))
      {
        supportedTypesPattern.insert(supportedTypesPattern.end(),
          filterDetailed.FilenamePatterns.begin(), filterDetailed.FilenamePatterns.end());
      }
    }
    supportedTypesDetailed->FilenamePatterns = supportedTypesPattern;
  }

  bool first = true;
  std::vector<ReaderPair> readerPairVector;
  // Generates the filter string used by the fileDialog
  // For example, this could be "Supported Files (*.jpg *.jpeg *.png);;All Files (*);;JPEG Image
  // Files(*.jpg *.jpeg);;PNG Image Files (*.png)"
  for (auto const& filterDetailed : filtersDetailed)
  {
    // Check if reader pair is part of provided reader pair list
    // only if list is not empty and not a standard description
    ReaderPair readerPair(
      QString::fromStdString(filterDetailed.Group), QString::fromStdString(filterDetailed.Name));
    if (readerSet.isEmpty() ||
      filterDetailed.Description == vtkSMReaderFactory::SUPPORTED_TYPES_DESCRIPTION ||
      filterDetailed.Description == vtkSMReaderFactory::ALL_FILES_DESCRIPTION ||
      readerSet.contains(readerPair))
    {
      if (!first)
      {
        filtersString += ";;";
      }

      filtersString += QString::fromStdString(filterDetailed.Description) + " (" +
        QString::fromStdString(vtksys::SystemTools::Join(filterDetailed.FilenamePatterns, " ")) +
        ")";
      readerPairVector.push_back(readerPair);
      first = false;
    }
  }

  int constexpr SupportedFilesFilterIndex = 0;
  int constexpr AllFilesFilterIndex = 1;

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Open File:"), QString(), filtersString);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFilesAndDirectories);
  QList<pqPipelineSource*> sources;
  if (fileDialog.exec() == QDialog::Accepted)
  {
    QList<QStringList> files = fileDialog.getAllSelectedFiles();
    int filterIndex = fileDialog.getSelectedFilterIndex();
    switch (filterIndex)
    {
      case SupportedFilesFilterIndex:
      {
        auto newSources = pqLoadDataReaction::loadFilesForSupportedTypes(files);
        for (auto const& source : newSources)
        {
          sources << source;
        }
      }
      break;
      case AllFilesFilterIndex:
      {
        auto newSources = pqLoadDataReaction::loadFilesForAllTypes(files, server, readerFactory);
        for (auto const& source : newSources)
        {
          sources << source;
        }
      }
      break;
      default:
        // Specific reader
        pqPipelineSource* source = pqLoadDataReaction::loadData(
          files, readerPairVector[filterIndex].first, readerPairVector[filterIndex].second);
        if (source)
        {
          sources << source;
        }
    }
  }
  return sources;
}

//-----------------------------------------------------------------------------
QVector<pqPipelineSource*> pqLoadDataReaction::loadFilesForSupportedTypes(QList<QStringList> files)
{
  QVector<pqPipelineSource*> newSources;
  auto* settings = vtkSMSettings::GetInstance();
  char const* settingName = ".settings.RepresentedArrayListSettings.ReaderDetails";

  for (auto const& file : files)
  {
    QFileInfo fileInfo(file[0]);
    bool loaded = false;
    unsigned int const numberOfEntries = settings->GetSettingNumberOfElements(settingName) / 3;
    for (unsigned int loopIndex = 0; loopIndex < numberOfEntries; ++loopIndex)
    {
      // Read entries from the end to the start
      unsigned int const reversedIndex = numberOfEntries - loopIndex - 1;
      auto const patternsString = settings->GetSettingAsString(settingName, reversedIndex * 3, "");
      std::vector<std::string> patterns;
      vtksys::SystemTools::Split(patternsString, patterns, ' ');

      for (std::string const& pattern : patterns)
      {
        if (QRegExp(QString::fromStdString(pattern), Qt::CaseInsensitive, QRegExp::Wildcard)
              .exactMatch(fileInfo.fileName()))
        {
          pqPipelineSource* source = pqLoadDataReaction::loadData(file, "sources",
            QString::fromStdString(
              settings->GetSettingAsString(settingName, reversedIndex * 3 + 2, "")));
          loaded = true;
          if (source)
          {
            newSources << source;
          }
          break;
        }
      }
      if (loaded)
      {
        break;
      }
    }

    if (!loaded)
    {
      pqPipelineSource* source = pqLoadDataReaction::loadData(QStringList(file));
      if (source)
      {
        newSources << source;
      }
    }
  }
  return newSources;
}

//-----------------------------------------------------------------------------
QVector<pqPipelineSource*> pqLoadDataReaction::loadFilesForAllTypes(
  QList<QStringList> files, pqServer* server, vtkSMReaderFactory* readerFactory)
{
  QVector<pqPipelineSource*> newSources;
  vtkStringList* list = readerFactory->GetReaders(server->session());
  for (QStringList const& fileGroups : files)
  {
    pqSelectReaderDialog prompt(fileGroups[0], server, list, pqCoreUtilities::mainWidget());
    if (prompt.exec() == QDialog::Accepted)
    {
      if (prompt.isSetAsDefault())
      {
        QString customPattern;
        QString const completeSuffix = QFileInfo(fileGroups[0]).completeSuffix();
        if (completeSuffix.isEmpty())
        {
          customPattern = "*";
        }
        else
        {
          customPattern = "*." + completeSuffix;
        }

        bool cancel = false;
        bool patternValid = false;
        QString errorString = "";
        // Ask the user the pattern they want to associate with the reader as long as the pattern is
        // not valid, or the user cancels the operation
        while (!patternValid)
        {
          bool ok;
          customPattern = QInputDialog::getText(&prompt, "Define custom pattern",
            QString(
              "Define the pattern you want to use to default to this reader.\nFor example: '*.png "
              "*.jpg'") +
              (errorString.isEmpty() ? "" : ("\nError: '" + errorString + "'")),

            QLineEdit::Normal, customPattern, &ok);

          if (!ok)
          {
            cancel = true;
            break;
          }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
          auto splitCustomPattern = customPattern.split(".", Qt::SkipEmptyParts);
#else
          auto splitCustomPattern = customPattern.split(" ", QString::SkipEmptyParts);
#endif
          patternValid = !splitCustomPattern.empty() &&
            std::all_of(
              splitCustomPattern.begin(), splitCustomPattern.end(), [&](QString const& pattern) {
                QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
                if (!regexp.isValid())
                {
                  errorString = regexp.errorString();
                  return false;
                }
                return true;
              });
          if (splitCustomPattern.empty())
          {
            errorString = "pattern cannot be empty";
          }
        }

        if (cancel)
        {
          continue;
        }

        pqLoadDataReaction::addReaderToDefaults(
          prompt.getReader(), server, readerFactory, customPattern);
      }
      pqPipelineSource* source =
        pqLoadDataReaction::loadData(files, prompt.getGroup(), prompt.getReader());
      if (source)
      {
        newSources << source;
      }
    }
  }
  return newSources;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::loadData(
  const QStringList& files, const QString& smgroup, const QString& smname, pqServer* server)
{
  QList<QStringList> f;
  QMap<std::string, QStringList> fileGroups;
  vtkNew<vtkFileSequenceParser> sequenceParser;
  // --- detect file series
  for (const auto& _file : files)
  {
    if (sequenceParser->ParseFileSequence(_file.toUtf8()))
    {
      fileGroups[sequenceParser->GetSequenceName()].append(_file);
    }
    else
    {
      f.append(QStringList(_file));
    }
  }
  f << fileGroups.values();
  return pqLoadDataReaction::loadData(f, smgroup, smname, server);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::loadData(
  const QList<QStringList>& files, const QString& smgroup, const QString& smname, pqServer* server)
{
  if (files.empty())
  {
    return nullptr;
  }

  server = server != nullptr ? server : pqActiveObjects::instance().activeServer();
  if (!server)
  {
    qCritical() << "Cannot create reader without an active server.";
    return nullptr;
  }

  vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  pqPipelineSource* reader = nullptr;

  // Extension to ReaderType,ReaderGroup Hash table
  QHash<QString, QPair<QString, QString>> extensionToReaderSelection;
  Q_FOREACH (const QStringList& filegroup, files)
  {
    QPair<QString, QString> readerInfo; // type,group
    QString filename(filegroup[0]);
    QFileInfo fi(filename);

    if (!pqLoadDataReaction::TestFileReadability(filename, server, readerFactory))
    {
      qWarning() << "File '" << filename << "' cannot be read.";
      continue;
    }

    if (!smgroup.isEmpty() && !smname.isEmpty())
    {
      readerInfo = QPair<QString, QString>(smname, smgroup);
    }
    else
    {
      // Determine reader type based on if we have asked the user for this extension before
      QHash<QString, QPair<QString, QString>>::const_iterator it =
        extensionToReaderSelection.find(fi.suffix());
      if (it != extensionToReaderSelection.end())
      {
        readerInfo = it.value();
      }
      else
      {
        // Determine reader type based on the first filegroup
        bool valid =
          pqLoadDataReaction::DetermineFileReader(filename, server, readerFactory, readerInfo);
        if (valid == 0)
        {
          // no reader selected
          continue;
        }
      }
    }

    // read the filegroup
    BEGIN_UNDO_SET("Create 'Reader'");
    reader = pqLoadDataReaction::LoadFile(filegroup, server, readerInfo);
    END_UNDO_SET();

    // add this extension to the hash set
    extensionToReaderSelection.insert(fi.suffix(), readerInfo);
  }
  return reader;
}

//-----------------------------------------------------------------------------
bool pqLoadDataReaction::TestFileReadability(
  const QString& file, pqServer* server, vtkSMReaderFactory* vtkNotUsed(factory))
{
  return vtkSMReaderFactory::TestFileReadability(file.toUtf8().data(), server->session());
}

//-----------------------------------------------------------------------------
bool pqLoadDataReaction::DetermineFileReader(const QString& filename, pqServer* server,
  vtkSMReaderFactory* factory, QPair<QString, QString>& readerInfo)
{
  QString readerType, readerGroup;
  vtkStringList* list = factory->GetReaders(filename.toUtf8().data(), server->session());
  if (list->GetLength() > 3)
  {
    // If more than one readers found.
    pqSelectReaderDialog prompt(filename, server, list, pqCoreUtilities::mainWidget());
    if (prompt.exec() == QDialog::Accepted)
    {
      if (prompt.isSetAsDefault())
      {
        pqLoadDataReaction::addReaderToDefaults(prompt.getReader(), server, factory);
      }
      readerType = prompt.getReader();
      readerGroup = prompt.getGroup();
    }
    else
    {
      // User didn't choose any reader.
      return false;
    }
  }
  else if (list->GetLength() == 3)
  {
    // reader knows the type
    readerGroup = list->GetString(0);
    readerType = list->GetString(1);
  }
  else
  {
    // The reader factory could not determine the type of reader to create for the
    // file. Ask the user.
    pqSelectReaderDialog prompt(filename, server, factory, pqCoreUtilities::mainWidget());
    if (prompt.exec() == QDialog::Accepted)
    {
      if (prompt.isSetAsDefault())
      {
        pqLoadDataReaction::addReaderToDefaults(prompt.getReader(), server, factory);
      }
      readerType = prompt.getReader();
      readerGroup = prompt.getGroup();
    }
    else
    {
      // User didn't choose any reader.
      return false;
    }
  }
  readerInfo.first = readerType;
  readerInfo.second = readerGroup;
  return true;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLoadDataReaction::LoadFile(
  const QStringList& files, pqServer* server, const QPair<QString, QString>& readerInfo)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineSource* reader =
    builder->createReader(readerInfo.second, readerInfo.first, files, server);

  if (reader)
  {
    pqStandardRecentlyUsedResourceLoaderImplementation::addDataFilesToRecentResources(
      server, files, reader->getProxy()->GetXMLGroup(), reader->getProxy()->GetXMLName());
  }
  return reader;
}

//-----------------------------------------------------------------------------
void pqLoadDataReaction::addReaderToDefaults(QString const& readerName, pqServer* server,
  vtkSMReaderFactory* readerFactory, QString const& customPattern)
{
  auto pxm = server ? server->proxyManager() : nullptr;
  auto settingsProxy = pxm
    ? vtkSMSettingsProxy::SafeDownCast(pxm->GetProxy("settings", "RepresentedArrayListSettings"))
    : nullptr;

  auto* settings = vtkSMSettings::GetInstance();
  char const* settingName = ".settings.RepresentedArrayListSettings.ReaderDetails";
  unsigned int numberOfEntries = settings->GetSettingNumberOfElements(settingName) / 3;

  std::string const readerNameString = readerName.toStdString();

  auto filtersDetailed = readerFactory->GetSupportedFileTypesDetailed(server->session());
  auto readerIt = std::find_if(filtersDetailed.begin(), filtersDetailed.end(),
    [&](FileTypeDetailed const& fileType) { return fileType.Name == readerNameString; });

  if (readerIt == filtersDetailed.end())
  {
    vtkGenericWarningMacro(<< "Could not add reader as default, name '" << readerName.toStdString()
                           << "' is incorrect.");
    return;
  }

  std::string customPatternString = customPattern.toStdString();
  if (customPatternString.empty())
  {
    customPatternString = vtksys::SystemTools::Join(readerIt->FilenamePatterns, " ");
  }

  settings->SetSetting(settingName, numberOfEntries * 3, customPatternString);
  vtkSMPropertyHelper(settingsProxy, "ReaderDetails")
    .Set(numberOfEntries * 3, customPatternString.c_str());
  settings->SetSetting(settingName, numberOfEntries * 3 + 1, readerIt->Description);
  vtkSMPropertyHelper(settingsProxy, "ReaderDetails")
    .Set(numberOfEntries * 3 + 1, readerIt->Description.c_str());
  settings->SetSetting(settingName, numberOfEntries * 3 + 2, readerNameString);
  vtkSMPropertyHelper(settingsProxy, "ReaderDetails")
    .Set(numberOfEntries * 3 + 2, readerNameString.c_str());
  settingsProxy->UpdateVTKObjects();
  numberOfEntries++;
}
