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
#ifndef pqOutputPortComboBox_h
#define pqOutputPortComboBox_h

#include "pqComponentsModule.h"
#include <QComboBox>

class pqOutputPort;
class pqPipelineSource;
class pqServerManagerModelItem;

/**
* pqOutputPortComboBox is a combo-box that shows all outputports for all
* sources/filters.
*/
class PQCOMPONENTS_EXPORT pqOutputPortComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqOutputPortComboBox(QWidget* parent = 0);
  ~pqOutputPortComboBox() override;

  /**
  * Enable/Disable changing of the combo-box selected index based on the
  * active source/port. Default is true i.e. enabled.
  */
  void setAutoUpdateIndex(bool val) { this->AutoUpdateIndex = val; }
  bool autoUpdateIndex() const { return this->AutoUpdateIndex; }

  /**
  * Makes is possible to add custom items to the combo-box.
  * \c port can be NULL.
  */
  void addCustomEntry(const QString& label, pqOutputPort* port);

  /**
  * Returns the currently selected output port.
  */
  pqOutputPort* currentPort() const;

  /**
  * May be called once after creation to initialize the widget with already
  * existing sources.
  */
  void fillExistingPorts();

public Q_SLOTS:
  /**
  * Sets the current index to be the one representing the indicated port.
  */
  void setCurrentPort(pqOutputPort* port);

Q_SIGNALS:
  /**
  * Fired when the current index changes.
  */
  void currentIndexChanged(pqOutputPort*);

private Q_SLOTS:
  /**
  * Called when a source's name might have changed.
  */
  void nameChanged(pqServerManagerModelItem* item);

  /**
  * Called when current in the server manager selection changes.
  */
  void portChanged(pqOutputPort* item);

  /**
  * Called when currentIndexChanged(int) is fired.
  * We fire currentIndexChanged(pqPipelineSource*) and
  */
  // currentIndexChanged(vtkSMProxy*);
  void onCurrentIndexChanged(int index);

  /**
  * Called when a new source is added.
  */
  void addSource(pqPipelineSource* source);

  /**
  * Called when a new source is removed.
  */
  void removeSource(pqPipelineSource* source);

protected:
  bool AutoUpdateIndex;

private:
  Q_DISABLE_COPY(pqOutputPortComboBox)
};

#endif
