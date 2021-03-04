/*=========================================================================

   Program: ParaView
   Module:    pqPythonFileIO.h

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

#ifndef pqPythonTextSave_h
#define pqPythonTextSave_h

#include "pqPythonModule.h"

#include "pqPythonUtils.h"

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
   * @returns false if the file is invalid
   */
  bool openFile(const QString& filename);

  /**
   * @brief Sets the default save directory
   */
  void setDefaultSaveDirectory(const QString& dir) { this->DefaultSaveDirectory = dir; }

  /**
   * @brief Returns the filename the editor acts on
   */
  const QString& getFilename() const { return this->File.Name; }

  /**
   * @brief Returns true if the buffer content has been saved on the disk
   */
  bool isDirty() const;

signals:
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
   * @brief Signals that the file has been saved under
   * the macro directory from Paraview
   */
  void fileSavedAsMacro(const QString&);

  /**
   * @brief Emitted when the content of the buffer
   * has changed
   */
  void contentChanged();

public slots:
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

private:
  struct PythonFile
  {
    PythonFile() = default;

    PythonFile(const QString& str, QTextEdit* textEdit)
      : Name(str)
      , Text(textEdit)
    {
    }

    bool operator!=(const PythonFile& other) const { return this->Name != other.Name; }

    bool writeToFile() const;

    bool readFromFile(QString& str) const;

    void start();

    void removeSwap() const;

    QString Name = "";
    QTextEdit* Text = nullptr;
  } File;

  bool saveBuffer(const QString& file);

  QTextEdit& TextEdit;

  /**
   * @brief The default save directory (ie ~/home)
   */
  QString DefaultSaveDirectory = QDir::homePath();
};

#endif // pqPythonTextSave_h
