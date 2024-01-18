// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTransferFunction2DWidget_h
#define pqTransferFunction2DWidget_h

#include "pqComponentsModule.h"
#include "vtkType.h"
#include <QWidget>

// Forward declarations
class vtkChart;
class vtkImageData;
class vtkPVTransferFunction2D;
// class vtkPVDiscretizableColorTransferFunction;

/**
 * pqTransferFunction2DWidget provides a widget that can edit control boxes in a 2D histogram to
 * represent a 2D transfer function.
 * It is used by the pqColorOpacityEditorWidget, for example, to show 2D transfer function editor
 * for color and opacity for a PVLookupTable proxy.
 */
class PQCOMPONENTS_EXPORT pqTransferFunction2DWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqTransferFunction2DWidget(QWidget* parent = nullptr);
  ~pqTransferFunction2DWidget() override;

  ///@{
  /**
   * Set/Get vtkImageData used to initialize the 2D histogram
   */
  vtkImageData* histogram() const;
  virtual void setHistogram(vtkImageData*);
  ///@}

  ///@{
  /**
   * Initialize the pqTransferFunction2DWidget with a default box item.
   */
  void initialize(vtkPVTransferFunction2D* tf2d);
  bool isInitialized();
  ///@}

  /**
   * Get access to the internal chart.
   */
  vtkChart* chart() const;

  /**
   * Get access to the 2D transfer function passed to initialize
   */
  vtkPVTransferFunction2D* transferFunction() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * re-renders the transfer function view. This doesn't render immediately,
   * schedules a render.
   */
  void render();

Q_SIGNALS:
  /**
   * Signal fired to indicate that the transfer function was modified either by
   * adding/deleting/editing 2D boxes.
   */
  void transferFunctionModified();

protected Q_SLOTS:
  /**
   * Show usage info in the application status bar.
   */
  void showUsageStatus();

private:
  Q_DISABLE_COPY(pqTransferFunction2DWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
