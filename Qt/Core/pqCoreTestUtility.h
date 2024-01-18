// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqCoreTestUtility_h
#define pqCoreTestUtility_h

#include "pqCoreModule.h"
#include "pqTestUtility.h"
#include <QSize>
#include <QStringList>
#include <vtkIOStream.h>

class pqEventPlayer;
class pqEventTranslator;
class pqView;
class QString;
class vtkImageData;
class vtkRenderWindow;

/**
 * Provides ParaView-specific functionality for regression testing
 */
class PQCORE_EXPORT pqCoreTestUtility : public pqTestUtility
{
  Q_OBJECT
  typedef pqTestUtility Superclass;

public:
  pqCoreTestUtility(QObject* parent = nullptr);
  ~pqCoreTestUtility() override;

  /**
   * Cleans up patch to replace $PARAVIEW_TEST_ROOT and $PARAVIEW_DATA_ROOT
   * with appropriate paths specified on the command line.
   */
  static QString fixPath(const QString& path);

  /**
   * Returns the absolute path to the PARAVIEW_DATA_ROOT in canonical form
   * (slashes forward), or empty string
   */
  static QString DataRoot();

  /**
   * Returns the temporary test directory in which tests can write
   * temporary outputs, difference images etc.
   */
  static QString TestDirectory();

  /**
   * Returns the baseline directory in which test recorder will write
   * baseline images.
   */
  static QString BaselineDirectory();

  /**
   * Saves the contents of a render window to a file for later use as a
   * reference image
   */
  static bool SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File);

  /**
   * Compares the contents of a render window to a reference image,
   * returning true iff the two match within a given threshold
   */
  static bool CompareImage(vtkRenderWindow* renderWindow, const QString& referenceImage,
    double threshold, ostream& output, const QString& tempDirectory,
    const QSize& size = QSize(300, 300));

  /**
   * Compares the test image to a reference image,
   * returning true iff the two match within a given threshold
   */
  static bool CompareImage(vtkImageData* testImage, const QString& ReferenceImage, double Threshold,
    ostream& Output, const QString& TempDirectory);

  static bool CompareImage(const QString& testPNGImage, const QString& ReferenceImage,
    double Threshold, ostream& Output, const QString& TempDirectory);

  /**
   * Compares the contents of any arbitrary QWidget to a reference image,
   * returning true iff the two match within a given threshold
   */
  static bool CompareImage(QWidget* widget, const QString& referenceImage, double threshold,
    ostream& output, const QString& tempDirectory, const QSize& size = QSize(300, 300));

  static bool CompareView(pqView* curView, const QString& referenceImage, double threshold,
    const QString& tempDirectory, const QSize& size = QSize());

  static const char* PQ_COMPAREVIEW_PROPERTY_NAME;

  static bool CompareTile(QWidget* widget, int rank, int tdx, int tdy, const QString& baseline,
    double threshold, ostream& output, const QString& tempDirectory);
  static bool CompareTile(pqView* widget, int rank, int tdx, int tdy, const QString& baseline,
    double threshold, ostream& output, const QString& tempDirectory);

  /**
   * Cleans a path string
   */
  static QString cleanPath(const QString&);

  /**
   * Reimplemented to inform this test utility supports dashboard mode
   * always returns true.
   */
  bool supportsDashboardMode() override { return true; };

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * put/unset the DASHBOARD_TEST_FROM_CTEST env var
   * to control if ParaView should behave in dashboard mode
   * Also update players and translators
   */
  void setDashboardMode(bool value) override;

  /**
   * Reimplemented to add/remove the file dialog player
   * according to the DASHBOARD_TEST_FROM_CTEST env var
   */
  void updatePlayers() override;

  /**
   * Reimplementated to add/remove the file dialog translator
   * according to the DASHBOARD_TEST_FROM_CTEST env var
   */
  void updateTranslators() override;

private:
  QStringList TestFilenames;
};

#endif // !pqCoreTestUtility_h
