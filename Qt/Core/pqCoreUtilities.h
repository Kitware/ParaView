// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCoreUtilities_h
#define pqCoreUtilities_h

#include "pqCoreModule.h"
#include "pqEventDispatcher.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>

class QPalette;
class vtkObject;

/**
 * INTERNAL CLASS (DO NOT USE). This is used by
 * pqCoreUtilities::connectWithVTK() methods.
 */
class PQCORE_EXPORT pqCoreUtilitiesEventHelper : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqCoreUtilitiesEventHelper(QObject* parent);
  ~pqCoreUtilitiesEventHelper() override;

Q_SIGNALS:
  void eventInvoked(vtkObject*, unsigned long, void*);

private:
  Q_DISABLE_COPY(pqCoreUtilitiesEventHelper)

  void executeEvent(vtkObject*, unsigned long, void*);
  class pqInternal;
  pqInternal* Interal;
  friend class pqCoreUtilities;
};

/**
 * pqCoreUtilities is a collection of arbitrary utility functions that can be
 * used by the application.
 */
class PQCORE_EXPORT pqCoreUtilities : public QObject
{
  Q_OBJECT
public:
  /**
   * When popping up dialogs, it's generally better if we set the parent
   * widget for those dialogs to be the QMainWindow so that the dialogs show up
   * centered correctly in the application. For that purpose this convenience
   * method is provided. It locates a QMainWindow and returns it.
   */
  static void setMainWidget(QWidget* widget) { pqCoreUtilities::MainWidget = widget; }
  static QWidget* mainWidget()
  {
    if (!pqCoreUtilities::MainWidget)
    {
      pqCoreUtilities::MainWidget = pqCoreUtilities::findMainWindow();
    }
    return pqCoreUtilities::MainWidget;
  }

  /**
   * Call QApplication::processEvents plus make sure the testing framework is
   */
  static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents)
  {
    pqEventDispatcher::processEvents(flags);
  }

  /**
   * Return the path of the root ParaView user specific configuration directory
   *
   * In disable registry mode (--dr), return the test directory instead.
   * If the test directory is empty, fall back to the usual user directory.
   */
  static QString getParaViewUserDirectory();

  /**
   * Return the path of the launched application
   */
  static QString getParaViewApplicationDirectory();

  /**
   * Return the AppData directory for Paraview.
   * Relies on Qt QStandardPaths::AppLocalDataLocation to get platform specific values.
   * The directory will created if needed.
   */
  static QString getParaViewApplicationDataDirectory();

  /**
   * Return the list of directories that can contains ParaView configurations.
   * First is the User Directory.
   * @see getParaViewUserDirectory(),  getApplicationDirectories()
   */
  static QStringList getParaViewApplicationConfigDirectories();

  /**
   * Return the list of full available path that exists inside the shared
   * application path and the user specific one
   * see getApplicationDirectories()
   */
  static QStringList findParaviewPaths(
    const QString& directoryOrFileName, bool lookupInAppDir, bool lookupInUserDir);

  /**
   * Return the list of possible directories for share application path and user directory.
   * App directories are computed from current application installation.
   * see getParaViewApplicationDirectory(), vtkPVStandardPaths::GetInstallDirectories()
   */
  static QStringList getApplicationDirectories(bool lookupInAppDir, bool lookupInUserDir);

  /**
   * Returns the first app directory found that contains the given relative path.
   * Return an empty string if not found.
   * Do not look in user space.
   *
   * @see getApplicationDirectories(true, false)
   */
  static QString findInApplicationDirectories(const QString& relativePath);

  static QString getNoneExistingFileName(QString expectedFilePath);

  /**
   * Method used to connect VTK events to Qt slots (or signals).
   * This is an alternative to using vtkEventQtSlotConnect. This method gives a
   * cleaner API to connect vtk-events to Qt slots. It manages cleanup
   * correctly i.e. either vtk-object or the qt-object can be deleted and the
   * observers will be cleaned up correctly. One can disconnect the connection
   * made explicitly by vtk_object->RemoveObserver(eventId) where eventId is
   * the returned value.
   */
  static unsigned long connect(vtkObject* vtk_object, int vtk_event_id, QObject* qobject,
    const char* signal_or_slot, Qt::ConnectionType type = Qt::AutoConnection);

  /**
   * This provides a mechanism to prompt the user to make a choice or
   * to show them some information. The user can decide to make their choice
   * persistent. Two configurations are supported:
   * 1. The method prompts the user with a "Yes", "No", and "Yes,
   * and don't ask again" message box. Returns true for "Yes" and false for "No".
   * If "Yes, and don't ask again" was clicked, the selection is remembered in
   * pqSettings and next time this method is called with the same settingsKey
   * it will simply return true.
   * 2. The method shows a message box with a "Ok" and "Ok and don't show again"
   * If "Ok, and don't show again" was clicked, the next time this method is
   * called with the same settingsKey it will simply return.
   * The 'don't ask/show again' button is created by or-ing 'buttons' with
   * QMessageBox::Save.
   * NOTE: due to issues with test recording and playback, currently, this
   * dialog is not prompted (instead always returning true) when
   * DASHBOARD_TEST_FROM_CTEST environment variable is set. This may change in future.
   */
  static bool promptUser(const QString& settingsKey, QMessageBox::Icon icon, const QString& title,
    const QString& message, QMessageBox::StandardButtons buttons, QWidget* parentWdg = nullptr);

  /**
   * This show a QMessageBox to the user and returns the clicked button.
   * NOTE: due to issues with test recording and playback, currently, this
   * dialog is not prompted (instead always returning true) when
   * DASHBOARD_TEST_FROM_CTEST environment variable is set. This may change in future.
   */
  static QMessageBox::Button promptUserGeneric(const QString& title, const QString& message,
    QMessageBox::Icon icon, QMessageBox::StandardButtons buttons, QWidget* parentWidget);

  /**
   * Converts a double value to a full precision QString.
   * Internally uses pqDoubleLineEdit with FullConversion, which relies on
   * `vtkNumberToString` for lossless conversion from double to string.
   * Set lowExponent and highExponent to control the range of exponents where
   * a fixed precision should be used instead of scientific notation.
   * Outside this range, scientific notation is preferred.
   * Default values of -6 and 20 correspond to the ECMAScript standard.
   */
  static QString number(double value, int lowExponent = -6, int highExponent = 20);

  /**
   * Convert double value to string according to FullNotation settings, rely on
   * pqCoreUtilities::number.
   */
  static QString formatFullNumber(double value);

  ///@{
  /**
   * Convert double value to string, handling formating.
   */
  // Format with given precision and notation and shortAccurate flag
  static QString formatDouble(double value, int notation, bool shortAccurate, int precision,
    int fullLowExponent = -6, int fullHighExponent = 20);

  // Format with RealNumberDisplayed settings
  static QString formatNumber(double value);

  // Format with AnimationTime settings
  static QString formatTime(double value);
  ///@}

  /**
   * Convert a double KiB value to an easily readable QString
   * For example, if this function is called with 1500.0 as an input and the default precision,
   * the QString "1.46 MiB" will be returned.
   */
  static QString formatMemoryFromKiBValue(double memoryInKB, int precision = 2);

  /**
   * Setups up appearance for buttons such as Apply button
   * that we want to draw user's attention to when enabled. Currently, this
   * changes the palette to use a green background when enabled.
   */
  static void initializeClickMeButton(QAbstractButton* button);

  /**
   * Set the Hightlight value of the palette to CDash green
   * Also set HightlightedText to black
   */
  static void setPaletteHighlightToOk(QPalette& palette);

  /**
   * Set the Hightlight value of the palette to CDash yellow
   * Also set HightlightedText to white
   */
  static void setPaletteHighlightToWarning(QPalette& palette);

  /**
   * Set the Hightlight value of the palette to CDash red
   * Also set HightlightedText to white
   */
  static void setPaletteHighlightToCritical(QPalette& palette);

  /**
   * Safely delete a directory recursively. This function indicates any errors with a modal popup
   * dialog.
   */
  static void removeRecursively(QDir dir);

  /**
   * Safely delete a file. This function indicates any errors
   * with a modal popup dialog.
   */
  static void remove(const QString& filePath);

  /**
   * Returns true if the current application is running in dark mode.
   * This is determined based on the current application palette.
   */
  static bool isDarkTheme();

private:
  static QWidget* findMainWindow();
  static QPointer<QWidget> MainWidget;
};

#endif
