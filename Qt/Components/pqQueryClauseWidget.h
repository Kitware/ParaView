/*=========================================================================

   Program: ParaView
   Module:    pqQueryClauseWidget.h

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
#ifndef pqQueryClauseWidget_h
#define pqQueryClauseWidget_h

#include "pqComponentsModule.h"
#include "vtkSelectionNode.h"
#include <QWidget>

class pqOutputPort;
class vtkPVDataSetAttributesInformation;
class vtkSMProxy;

/**
* pqQueryClauseWidget is used by pqQueryDialog as the widget allowing the user
* choose the clauses for the queries.
*/
class PQCOMPONENTS_EXPORT pqQueryClauseWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  enum CriteriaType
  {
    INVALID = 0x0,
    INDEX = 0x1,
    GLOBALID = 0x2,
    THRESHOLD = 0x4,
    BLOCK = 0x10,
    AMR_LEVEL = 0x20,
    AMR_BLOCK = 0x40,
    PROCESSID = 0x80,
    QUERY = 0x8,
    ANY = 0xffff
  };

  Q_DECLARE_FLAGS(CriteriaTypes, CriteriaType);

  enum ConditionMode
  {
    SINGLE_VALUE,
    SINGLE_VALUE_GE,
    SINGLE_VALUE_LE,
    PAIR_OF_VALUES,
    TRIPLET_OF_VALUES,
    LIST_OF_VALUES,
    BLOCK_ID_VALUE,
    LIST_OF_BLOCK_ID_VALUES,
    BLOCK_NAME_VALUE,
    AMR_LEVEL_VALUE,
    AMR_BLOCK_VALUE,
    SINGLE_VALUE_MIN,
    SINGLE_VALUE_MAX,
    SINGLE_VALUE_LE_MEAN,
    SINGLE_VALUE_GE_MEAN,
    SINGLE_VALUE_MEAN_WITH_TOLERANCE
  };

public:
  pqQueryClauseWidget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  virtual ~pqQueryClauseWidget();

  /**
  * Set/Get the data producer.
  */
  void setProducer(pqOutputPort* p) { this->Producer = p; }
  pqOutputPort* producer() const { return this->Producer; }

  /**
  * Set the attribute type. This determine what arrays are listed in the
  * selection criteria.
  * Valid values are from the enum vtkDataObject::AttributeTypes.
  */
  void setAttributeType(int attrType) { this->AttributeType = attrType; }
  int attributeType() const { return this->AttributeType; }

  /**
  * Creates a new selection source proxy based on the query.
  * Note that this does not register the proxy, it merely creates the
  * selection source and returns it.
  */
  vtkSMProxy* newSelectionSource();

public slots:
  /**
  * use this slot to initialize the clause GUI after all properties have been
  * set.
  * FIXME: Unsupported Terms: process id, amr-level, amr-block. We will need
  * to extend VTK selection support for those, so we will implement them later
  * (possibly 3.10/4.0)
  */
  void initialize()
  {
    this->initialize(CriteriaTypes(ANY) ^ PROCESSID ^ AMR_LEVEL ^ AMR_BLOCK ^ BLOCK);
  }

  /**
  * initialize the widget only with the subset of criteria mentioned.
  * A query clause has two components, the query term and the qualifiers. Some
  * criteria can be both eg. BLOCK can be both the term or a qualifier. The
  * available operators may change in such case. Hence, we specify if it's
  * being used as a qualifier or not.
  */
  void initialize(CriteriaTypes type_flags, bool qualifier_mode = false);

signals:
  /**
  * Fired when the user clicks on the help button.
  */
  void helpRequested();

protected slots:
  /**
  * Based on the selection criteria, populate the options in the selection
  * "condition" combo box.
  */
  void populateSelectionCondition();

  /**
  * Update the value widget so show the correct widget based on the chosen
  * criteria and condition.
  */
  void updateValueWidget();

  /**
  * Some query clauses under certain conditions require additional options
  * from the user. These are managed using more instances of
  * pqQueryClauseWidget internally. This method creates these new instances if
  * needed.
  */
  void updateDependentClauseWidgets();

  /**
  * Pops up a dialog showing the composite data structure for the data.
  */
  void showCompositeTree();

protected:
  /**
  * Returns the attribute info for the attribute chosen in the "Selection
  * Type" combo box.
  */
  vtkPVDataSetAttributesInformation* getChosenAttributeInfo() const;

  /**
  * Based on the selection type and data information from the producer,
  * populate the "criteria" combo box.
  */
  void populateSelectionCriteria(CriteriaTypes type = ANY);

  /**
  * Returns the current criteria type.
  */
  CriteriaType currentCriteriaType() const;

  ConditionMode currentConditionType() const;

  /**
  * Updates the selection source proxy with the criteria in the clause.
  */
  void addSelectionQualifiers(vtkSMProxy*);

  pqOutputPort* Producer;
  int AttributeType;
  bool AsQualifier;
  QString LastQuery;

private:
  Q_DISABLE_COPY(pqQueryClauseWidget)
  class pqInternals;
  pqInternals* Internals;
};

#endif
