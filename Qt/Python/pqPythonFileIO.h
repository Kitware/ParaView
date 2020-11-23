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

#include <QAction>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTextEdit>

#include <array>

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
   * @brief All the \ref pqPythonFileIO QAction
   * provided by this class.
   *
   * @details Each action is connected to
   * the proper functionality at construction
   * (ie there is no need to connect them
   * when using this class)
   */
  enum class IOAction : std::uint8_t
  {
    NewFile = 0,
    OpenFile,
    SaveFile,
    SaveFileAs,
    SaveFileAsMacro,
    END
  };

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
   * @brief Get one of the action listed in \ref IOAction
   * @param[in] action the wanted action
   * @returns a pointer to the QAction
   */
  QAction* GetAction(const IOAction action) { return &Actions[action]; }

  /**
   * @brief Saves and close the underlying file
   * @return true if the saving process was successful,
   * false if the user discarded the saves or if something
   * wrong happened during file I/O
   */
  bool SaveOnClose();

  /**
   * @brief Opens and load the given file
   * @param[in] filename the file to be opened
   * @returns false if the file is invalid
   */
  bool OpenFile(const QString& filename);

  /**
   * @brief Sets the default save directory
   */
  void SetDefaultSaveDirectory(const QString& dir) { this->DefaultSaveDirectory = dir; }

signals:
  /**
   * @brief Signals that the QTextEdit buffer has been erased
   */
  void BufferErased();

  /**
   * @brief Signals that a file has been opened
   */
  void FileOpened(const QString&);

  /**
   * @brief Signals that the file has been saved
   */
  void FileSaved(const QString&);

  /**
   * @brief Signals that the file has been saved under
   * the macro directory from Paraview
   */
  void FileSavedAsMacro(const QString&);

private slots:
  /**
   * @brief Creates a new file
   * @details Save the current buffer if needed
   * and clears the underlying \ref TextEdit
   */
  bool NewFile();

  /**
   * @brief Opens a new file
   * @details Ask the user which file to open
   */
  bool Open();

  /**
   * @brief Saves the underlying file
   * @details If no file is associated, ask the
   * user which file to save this buffer in
   */
  bool Save();

  /**
   * @brief Saves the current file under a new file
   * and opens it in the editor
   */
  bool SaveAs();

  /**
   * @brief Saves the current file under the macro directory
   */
  bool SaveAsMacro();

private:
  bool LoadFile(const QString& filename);

  bool SaveFile(const QString& filename);

  /**
   * @brief Internal utility function that tells if this buffer needs to be saved
   * @details For now, we use the modified property of the QTextDocument
   */
  bool NeedSave() const { return this->TextEdit.document()->isModified(); }

  void SetModified(bool modified) { this->TextEdit.document()->setModified(modified); }

  QTextEdit& TextEdit;

  /**
   * @brief The default save directory (ie ~/home)
   */
  QString DefaultSaveDirectory = QDir::homePath();

  QString Filename = "";

  /**
   * @class EnumArray
   * @brief Static array accessible through an enum
   */
  template <typename E, class T, std::size_t N = static_cast<size_t>(E::END)>
  struct EnumArray : public std::array<T, N>
  {
    T& operator[](E e) { return std::array<T, N>::operator[](static_cast<size_t>(e)); }
  };

  /**
   * @brief The list of QAction
   * listed in \ref IOAction
   */
  EnumArray<IOAction, QAction> Actions;
};

#endif // pqPythonTextSave_h
