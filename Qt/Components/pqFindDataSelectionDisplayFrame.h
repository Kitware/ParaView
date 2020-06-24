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
#ifndef pqFindDataSelectionDisplayFrame_h
#define pqFindDataSelectionDisplayFrame_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOutputPort;
class pqView;

/**
* pqFindDataSelectionDisplayFrame is designed to be used by pqFindDataDialog.
* pqFindDataDialog uses this class to allow controlling the display properties
* for the selection in the active view. Currently, it only support
* controlling the display properties for the selection in a render view.
* It monitors the active selection by tracking pqSelectionManager as well as
* the active view by tracking pqActiveObjects singleton.
*/
class PQCOMPONENTS_EXPORT pqFindDataSelectionDisplayFrame : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(bool useVerticalLayout READ useVerticalLayout WRITE setUseVerticalLayout)

  typedef QWidget Superclass;

public:
  pqFindDataSelectionDisplayFrame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqFindDataSelectionDisplayFrame() override;

  /**
  * pqFindDataSelectionDisplayFrame can be made to lay itself out in a more
  * vertical fashion rather than the default, horizontal layout. To use a
  * vertical layout, use this method.
  */
  void setUseVerticalLayout(bool);
  bool useVerticalLayout() const;

public Q_SLOTS:
  /**
  * Set the output port that is currently selected for which we are
  * controlling the selection display properties.
  */
  void setSelectedPort(pqOutputPort*);

  /**
  * set the view in which we are controlling the selection display properties.
  * label properties as well as which array to label with affect only the
  * active view.
  */
  void setView(pqView*);

private Q_SLOTS:
  void updatePanel();
  void fillCellLabels();
  void fillPointLabels();
  void cellLabelSelected(QAction*);
  void pointLabelSelected(QAction*);
  void editLabelPropertiesSelection();
  void editLabelPropertiesInteractiveSelection();
  void showFrustum(bool);
  void onDataUpdated();

  /**
  * List for selection changes and enable/disable UI elements as appropriate.
  * \c frustum indicates whether the selection is frustum-based or not.
  */
  void onSelectionModeChanged(bool frustum);

private:
  Q_DISABLE_COPY(pqFindDataSelectionDisplayFrame)

  void updateInteractiveSelectionLabelProperties();

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
};

#endif
