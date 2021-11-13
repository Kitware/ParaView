/*=========================================================================

   Program: ParaView
   Module:    pqPythonTabWidget.h

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

#ifndef pqPythonTabWidget_h
#define pqPythonTabWidget_h

#include "pqPythonModule.h"

#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QTabWidget>

class pqPythonTextArea;
struct pqPythonEditorActions;

/**
 * @class pqPythonTabWidget
 * @brief Encapsulates the multitab python editor
 * @details Provides a QWidget with multiple QTextArea
 * to edit python scripts in paraview. The widget handles
 * automatic saving, syntax highlighting and undo/redo
 * features within each tab.
 */
class PQPYTHON_EXPORT pqPythonTabWidget : public QTabWidget
{
  Q_OBJECT

public:
  /**
   * @brief Default contructor
   * @param[in] parent the QWidget owning this widget
   */
  pqPythonTabWidget(QWidget* parent);

  /**
   * @brief Returns the current displayed \ref pqPythonTextArea
   */
  pqPythonTextArea* getCurrentTextArea() const;

  /**
   * @brief Connects this class to the actions
   */
  void connectActions(pqPythonEditorActions& actions);

  /**
   * @brief Connects the current \ref pqPythonTextArea
   * to the actions
   */
  void updateActions(pqPythonEditorActions& actions);

  /**
   * @brief Returns true if all opened buffer
   * were saved on the disk.
   */
  bool saveOnClose();

  /**
   * @brief Update the text in the paraview trace
   * tab. Creates a new one if the tab doesn't exist.
   */
  void updateTrace(const QString& str);

  /**
   * @brief Update the text in the paraview python
   * trace tab and converts it into a unamed script.
   * If no tab exists, creates a new one.
   */
  void stopTrace(const QString& str);

  /**
   * @brief Link the input QTextEdit to
   * one of the tab of the editor.
   * If this objects is already linked within
   * the editor, switch to that tab otherwise creates
   * a new one
   */
  template <typename T>
  void linkTo(T* /*obj*/)
  {
    static_assert(sizeof(T) == 0, "Only specializations of linkTo(T* t) can be used");
  }

  void loadFile(const QString& filename);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * @brief Add a new empty text area
   */
  void createNewEmptyTab();

  /**
   * @brief Loads the content of the file
   * into the current tab if it's empty
   * and not linked to another QText.
   * Otherwise adds a new tab.
   */
  void addNewTextArea(const QString& filename);

  /**
   * @brief Request to close the current tab
   */
  void closeCurrentTab();

Q_SIGNALS:
  /**
   * @brief Raised when a file has been opened and loaded
   * into the text edit widget
   */
  void fileOpened(const QString&);

  /**
   * @brief Raised when a file has been saved successfuly
   */
  void fileSaved(const QString&);

protected:
  void keyPressEvent(QKeyEvent* keyEvent) override;
  void mousePressEvent(QMouseEvent* mouseEvent) override;

private:
  void updateTab(QWidget* widget);
  void addNewTabWidget();
  void setTabCloseButton(pqPythonTextArea* widget);
  void createParaviewTraceTab();
  void generateTabName(const pqPythonTextArea* widget, QString& tabName, QString& elidedTabName,
    QString& unstyledTabName) const;

  /**
   * @brief Returns -1 if the editor doesn't contain
   * this file. Otherwise returns the index
   */
  int fileIsOpened(const QString& filename) const;

  template <typename T>
  T* getWidget(int idx) const
  {
    if (idx < 0 || idx >= this->count() - 1)
    {
      return nullptr;
    }

    return reinterpret_cast<T*>(this->widget(idx));
  }

  pqPythonTextArea* lastFocus = nullptr;

  pqPythonTextArea* TraceWidget = nullptr;
};

#include "pqPythonTabWidget.txx"

#endif // pqPythonTabWidget_h
