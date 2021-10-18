/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
  pqTransferFunction2DWidget(QWidget* parent = 0);
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

public Q_SLOTS:
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

protected:
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
