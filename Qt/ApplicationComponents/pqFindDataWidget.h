/*=========================================================================

   Program: ParaView
   Module:  pqFindDataWidget.h

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
#ifndef pqFindDataWidget_h
#define pqFindDataWidget_h

#include "pqApplicationComponentsModule.h" // for exports
#include <QScopedPointer>                  // for QScopedPointer
#include <QWidget>

/**
 * @class pqFindDataWidget
 * @brief Widget to find data using selection queries.
 *
 * @section DeveloperNotes Developer Notes
 *
 * * Currently, this simply uses `pqFindDataCurrentSelectionFrame` to show
 *   current selection. We should modernize/cleanup that code and maybe just
 *   merge that code with this class to avoid confusion.
 *
 * * Currently, this simply uses `pqFindDataSelectionDisplayFrame` to allow
 *   editing current selection's display properties. We should modernize that
 *   code and maybe just move it to this class to avoid confusion.
 */

class pqServer;
class pqPipelineSource;
class pqOutputPort;

class PQAPPLICATIONCOMPONENTS_EXPORT pqFindDataWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqFindDataWidget(QWidget* parent = nullptr);
  ~pqFindDataWidget() override;

  pqServer* server() const;

public Q_SLOTS:
  /**
   * Set the server to use.
   */
  void setServer(pqServer* server);

private Q_SLOTS:
  /**
   * Ensure we don't have any references to ab proxy being deleted.
   */
  void aboutToRemove(pqPipelineSource*);

private:
  Q_DISABLE_COPY(pqFindDataWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  void handleSelectionChanged(pqOutputPort*);
};

#endif
