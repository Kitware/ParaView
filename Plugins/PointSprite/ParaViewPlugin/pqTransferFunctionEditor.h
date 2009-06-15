/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqTransferFunctionEditor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqTransferFunctionEditor
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#ifndef __pqTransferFunctionEditor_h
#define __pqTransferFunctionEditor_h

#include <QWidget>

class pqPipelineRepresentation;

class pqTransferFunctionEditor: public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqTransferFunctionEditor();
  ~pqTransferFunctionEditor();

  enum EditorConfiguration
  {
    Opacity, Radius
  };

  void configure(EditorConfiguration);

  void setRepresentation(pqPipelineRepresentation* repr);

public slots :
  void  needReloadGUI();

protected slots :
  void reloadGUI();

  void onFreeFormToggled(bool);

  void onProportionnalToggled(bool);
  void onProportionnalEdited();

  void onAutoScalarRange(bool);
  //void onAutoScaleRange(bool);

  void onScalarRangeModified();
  void onScaleRangeModified();

  void onTableValuesModified();
  void onGaussianValuesModified();

  void updateAllViews();

protected:
  QList<QVariant> freeformValues();
  QList<QVariant> gaussianControlPoints();

  void setFreeformValues(const QList<QVariant>&);
  void setGaussianControlPoints(const QList<QVariant>&);

  QList<QVariant> GetProxyValueList(const char *name);
  void SetProxyValue(const char *name, QList<QVariant> val, bool update = true);

  //void  initialize();

private:
  class pqInternals;
  pqInternals* Internals;

  pqTransferFunctionEditor(const pqTransferFunctionEditor&); // Not implemented.
  void operator=(const pqTransferFunctionEditor&); // Not implemented.
};

#endif

