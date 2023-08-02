// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileChooserWidget_h
#define pqFileChooserWidget_h

#include "pqComponentsModule.h"
#include "pqQtDeprecated.h"

#include <QString>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class pqServer;

/**
 * @class pqFileChooserWidget
 * @brief input widget for files
 *
 * pqFileChooserWidget which consists of a tool button and a line edit
 * hitting the tool button will bring up a file dialog, and the chosen
 * file will be put in the line edit
 */
class PQCOMPONENTS_EXPORT pqFileChooserWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QStringList filenames READ filenames WRITE setFilenames USER true)
  Q_PROPERTY(QString singleFilename READ singleFilename WRITE setSingleFilename)
  Q_PROPERTY(QString extension READ extension WRITE setExtension)
  Q_PROPERTY(bool useDirectoryMode READ useDirectoryMode WRITE setUseDirectoryMode)
  Q_PROPERTY(bool forceSingleFile READ forceSingleFile WRITE setForceSingleFile)
  Q_PROPERTY(bool acceptAnyFile READ acceptAnyFile WRITE setAcceptAnyFile)

public:
  /**
   * constructor
   */
  pqFileChooserWidget(QWidget* p = nullptr);
  /**
   * destructor
   */
  ~pqFileChooserWidget() override;

  /**
   * get the filename
   */
  QStringList filenames() const;
  /**
   * set the filename
   */
  // this doesn't verify that any of the filenames being passed
  // in are valid for the mode or actually exist on the server. That
  // must be down be what ever calls this method
  void setFilenames(const QStringList&);

  ///@{
  /**
   * Convenience functions for when using only a single file (see
   * forceSingleFile property).
   */
  QString singleFilename() const;
  void setSingleFilename(const QString&);
  ///@}

  /**
   * get the file extension for the file dialog
   */
  QString extension();
  /**
   * set the file extension for the file dialog
   */
  void setExtension(const QString&);

  /**
   * flag specifying whether this widget should accept multiple files
   */
  bool forceSingleFile() { return this->ForceSingleFile; }
  void setForceSingleFile(bool flag)
  {
    this->ForceSingleFile = flag;
    this->setFilenames(this->filenames());
  }

  /**
   * flag specifying whether this widget should use directory mode
   */
  bool useDirectoryMode() { return this->UseDirectoryMode; }
  void setUseDirectoryMode(bool flag)
  {
    this->UseDirectoryMode = flag;
    this->setFilenames(this->filenames());
  }

  /**
   * flag specifying whether this widget should accept any file
   */
  bool acceptAnyFile() { return this->AcceptAnyFile; }
  void setAcceptAnyFile(bool flag)
  {
    this->AcceptAnyFile = flag;
    this->setFilenames(this->filenames());
  }

  ///@{
  /**
   * Get/set the title to use. If an empty string is specified, a default one is
   * created.
   */
  void setTitle(const QString& ttle) { this->Title = ttle; }
  const QString& title() const { return this->Title; }
  ///@}

  /**
   * set server to work on.
   * If server is nullptr, a local file dialog is used
   */
  void setServer(pqServer* server);
  pqServer* server();

  /**
   * Converts between a list of file names and delimited string of filenames
   * (which is shown in the line edit box).
   */
  static QStringList splitFilenames(const QString& filesString)
  {
    return filesString.split(";", PV_QT_SKIP_EMPTY_PARTS);
  }
  static QString joinFilenames(const QStringList& filesList) { return filesList.join(";"); }

Q_SIGNALS:
  /**
   * Signal emitted when the filename changes.  The single string version is a
   * convenience for when you are only grabbing the first file anyway.
   */
  void filenamesChanged(const QStringList&);
  void filenameChanged(const QString&);

protected Q_SLOTS:
  /**
   * Called when the user hits the choose file button.
   */
  void chooseFile();
  /**
   * Respond to changes with the filename in the line edit box.
   */
  void handleFileLineEditChanged(const QString& fileString);

protected: // NOLINT(readability-redundant-access-specifiers)
  QString Extension;
  QLineEdit* LineEdit;
  pqServer* Server;
  bool ForceSingleFile;
  bool UseDirectoryMode;
  bool AcceptAnyFile;
  QStringList FilenameList;
  bool UseFilenameList;
  QString Title;

  /**
   * Takes a string with delimited files and emits the filenamesChanged
   *  and filenameChanged signals.
   */
  void emitFilenamesChanged(const QStringList& fileList);
};

#endif // pqFileChooserWidget_h
