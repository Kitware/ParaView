/*=========================================================================

   Program: ParaView
   Module:    pqOutputPortComboBox.h

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

========================================================================*/
#ifndef __pqOutputPortComboBox_h 
#define __pqOutputPortComboBox_h

#include <QComboBox>
#include "pqComponentsExport.h"

class pqOutputPort;
class pqPipelineSource;
class pqServerManagerModelItem;

/// pqOutputPortComboBox is equivalent to pqSourceComboBox except that instead
/// of shows all the source/filter proxies, it shows the output ports of all the
/// sourcs/filters.
class PQCOMPONENTS_EXPORT pqOutputPortComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;
public:
  pqOutputPortComboBox(QWidget* parent=0);
  ~pqOutputPortComboBox();

  /// Returns the currently selected output port.
  pqOutputPort* currentPort() const;

signals:
  /// Fired when the current index changes.
  void currentIndexChanged(pqOutputPort*);

private slots:
  /// Called when a source's name might have changed.
  void nameChanged(pqServerManagerModelItem* item);

  /// Called when current in the server manager selection changes.
  void onCurrentChanged(pqServerManagerModelItem* item);

  /// Called when currentIndexChanged(int) is fired.
  /// We fire currentIndexChanged(pqPipelineSource*) and
  //currentIndexChanged(vtkSMProxy*);
  void onCurrentIndexChanged(int index);

  /// Called when a new source is added.
  void addSource(pqPipelineSource* source);

  /// Called when a new source is removed.
  void removeSource(pqPipelineSource* source);

private:
  pqOutputPortComboBox(const pqOutputPortComboBox&); // Not implemented.
  void operator=(const pqOutputPortComboBox&); // Not implemented.
};

#endif


