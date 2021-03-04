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
 * @brief A simple close label that mimics
 * the paraview cross button to close a tab
 * in the main window.
 */
class PQPYTHON_EXPORT QCloseLabel : public QLabel
{
  Q_OBJECT

public:
  /**
   * @brief Default constructor is deleted
   */
  QCloseLabel() = delete;

  /**
   * @brief Default constructor that sets up the tooltip
   * and the label pixmap.
   */
  QCloseLabel(QWidget* w, QWidget* parent)
    : QLabel(parent)
    , widget(w)
  {
    this->setToolTip("Close tab");
    this->setStatusTip("Close tab");
    this->setPixmap(this->style()->standardIcon(QStyle::SP_TitleBarCloseButton).pixmap(16, 16));
  }

  /**
   * @brief Defaulted destructor for polymorphism
   */
  ~QCloseLabel() override = default;

signals:
  /**
   * @brief Signal emitted when the label
   * is clicked (to mimic a push button)
   * @param[in] w the widget attached to
   * the QCloseLabel
   */
  void onClicked(QWidget* w);

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    emit this->onClicked(widget);
    event->accept();
  }

private:
  QWidget* widget = nullptr;
};

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

public slots:
  /**
   * @brief Add a new empty text area
   */
  void createNewEmptyTab();

  /**
   * @brief Adds a new tab and opens the given
   * file.
   */
  void addNewTextArea(const QString& filename);

  /**
   * @brief Request to close the current tab
   */
  void closeCurrentTab();

signals:
  /**
   * @brief Raised when a file has been opened and loaded
   * into the text edit widget
   */
  void fileOpened(const QString&);

  /**
   * @brief Raised when a file has been saved successfuly
   */
  void fileSaved(const QString&);

  /**
   * @brief Raised when a file has been successfuly
   * saved as a paraview macro
   */
  void fileSavedAsMacro(const QString&);

protected:
  void keyPressEvent(QKeyEvent* keyEvent) override;
  void mousePressEvent(QMouseEvent* mouseEvent) override;

private:
  void updateTab(QWidget* widget);
  void addNewTabButton();
  void setTabCloseButton(pqPythonTextArea* widget);
  void createParaviewTraceTab();
  QLabel* getTabLabel(const pqPythonTextArea* widget);

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

#endif // pqPythonTabWidget_h
