// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPythonFileIO_h
#define pqPythonFileIO_h

#include "pqPythonModule.h"

#include "pqPythonUtils.h"

#include "vtkType.h" // For vtkTypeUInt32

#include <QAction>
#include <QDir>
#include <QObject>
#include <QString>

#include <iostream>

class QTextEdit;

/**
 * @class pqPythonFileIO
 * @brief Handles loading (resp. saving) from (resp. to) the disk to
 * (resp. from) a QTextEdit widget.
 */
class PQPYTHON_EXPORT pqPythonFileIO : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief Default constructor is not valid for this class
   */
  pqPythonFileIO() = delete;

  /**
   * @brief Construct a pqPythonFileIO
   * @param[in] parent the parent for the Qt hierarchy
   * @param[in] text the QTextEdit this object acts on
   */
  pqPythonFileIO(QWidget* parent, QTextEdit& text);

  /**
   * @brief Destroy this object.
   * @details Effectively clears the swap
   * created by this class
   */
  ~pqPythonFileIO() override;

  /**
   * @brief Saves and close the underlying file
   * @return true if the saving process was successful,
   * false if the user discarded the saves or if something
   * wrong happened during file I/O
   */
  bool saveOnClose();

  /**
   * @brief Opens and load the given file.
   * @param[in] filename the file to be opened
   * @param[in] location the location of the file
   * @returns false if the file is invalid
   */
  bool openFile(const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * @brief Sets the default save directory
   */
  void setDefaultSaveDirectory(const QString& dir) { this->DefaultSaveDirectory = dir; }

  /**
   * @brief Returns the filename the editor acts on
   */
  const QString& getFilename() const { return this->File.Name; }

  /**
   * @brief Returns the location of the file that the editor acts on
   */
  vtkTypeUInt32 getLocation() const { return this->File.Location; }

  /**
   * @brief Returns true if the buffer content has been saved on the disk
   */
  bool isDirty() const;

Q_SIGNALS:
  /**
   * @brief Signals that the QTextEdit buffer has been erased
   */
  void bufferErased();

  /**
   * @brief Signals that a file has been opened
   */
  void fileOpened(const QString&);

  /**
   * @brief Signals that the file has been saved
   */
  void fileSaved(const QString&);

  /**
   * @brief Emitted when the content of the buffer
   * has changed
   */
  void contentChanged();

public Q_SLOTS:
  /**
   * @brief Change the buffer status to modified
   */
  void setModified(bool modified);

  /**
   * @brief Saves the underlying file
   * @details If no file is associated, ask the
   * user which file to save this buffer in
   */
  bool save();

  /**
   * @brief Saves the current file under a new file
   * and opens it in the editor
   */
  bool saveAs();

  /**
   * @brief Saves the current file under the macro directory
   */
  bool saveAsMacro();

  /**
   * @brief Saves the current file under the script directory
   */
  bool saveAsScript();

private:
  struct PythonFile
  {
    PythonFile() = default;

    PythonFile(const QString& str, const vtkTypeUInt32 location, QTextEdit* textEdit)
      : Name(str)
      , Location(location)
      , Text(textEdit)
    {
    }

    bool operator!=(const PythonFile& other) const { return this->Name != other.Name; }

    bool writeToFile() const;

    bool readFromFile(QString& str) const;

    void start();

    void removeSwap() const;

    QString Name = "";
    vtkTypeUInt32 Location = 0x10 /*vtkPVSession::CLIENT*/;
    QTextEdit* Text = nullptr;
  } File;

  bool saveBuffer(const QString& file, vtkTypeUInt32 location);

  QTextEdit& TextEdit;

  /**
   * @brief The default save directory (ie ~/home)
   */
  QString DefaultSaveDirectory = QDir::homePath();
};

#endif // pqPythonFileIO_h
