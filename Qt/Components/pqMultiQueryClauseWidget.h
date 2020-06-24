/*=========================================================================

   Program: ParaView
   Module:    pqMultiQueryClauseWidget.h

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
#ifndef pqMultiQueryClauseWidget_h
#define pqMultiQueryClauseWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOutputPort;
class pqQueryClauseWidget;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMProxy;
class QScrollArea;

/**
 * pqMultiQueryClauseWidget is used by pqQueryDialog as the widget allowing the user
 * choose the clauses for the queries.
 */
class PQCOMPONENTS_EXPORT pqMultiQueryClauseWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqMultiQueryClauseWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
  ~pqMultiQueryClauseWidget() override = default;

  //@{
  /**
   * Set/Get the data producer.
   */
  void setProducer(pqOutputPort* p) { this->Producer = p; }
  pqOutputPort* producer() const { return this->Producer; }
  //@}

  //@{
  /**
   * Set the attribute type. This determine what arrays are listed in the
   * selection criteria.
   * Valid values are from the enum vtkDataObject::AttributeTypes.
   */
  void setAttributeType(int attrType) { this->AttributeType = attrType; }
  int attributeType() const { return this->AttributeType; }
  //@}

  /**
   * Initialize the widget : remove all existent pqQueryClauseWidgets
   * and create an empty one. Add relevant dependant clause widget.
   */
  void initialize();

  /**
   * return true if input data is of type AMR.
   */
  bool isAMR();

  /**
   * return true if input data is of type MultiBlock.
   */
  bool isMultiBlock();

  /**
   * Creates a new selection source proxy based on the query.
   * Note that this does not register the proxy, it merely creates the
   * selection source and returns it.
   */
  vtkSMProxy* newSelectionSource();

  /**
   * Returns the attribute info for the current attribute type.
   */
  vtkPVDataSetAttributesInformation* getChosenAttributeInfo() const;

protected Q_SLOTS:
  /**
   * Adds a default query clause widget.
   */
  void addQueryClauseWidget();

  /**
   * Update visibility and connections.
   */
  void onChildrenChanged();

protected:
  /**
   * Some query clauses under certain conditions require additional options
   * from the user. For instance, if input is a multiblock we add a BLOCKID clause.
   */
  void updateDependentClauseWidgets();

  /**
   * Adds a query clause widget.
   * @param type refers to criteria (see pqQueryClauseWidget::CriteriaType).
   */
  void addQueryClauseWidget(int type, bool qualifier_mode);

  pqOutputPort* Producer;
  int AttributeType;
  int ChildNextId;
  int NumberOfDependentClauseWidgets;
  bool AddingClauseWidget;

  QWidget* Container;
  QScrollArea* ScrollArea;

private:
  Q_DISABLE_COPY(pqMultiQueryClauseWidget)
};

#endif
