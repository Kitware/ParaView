// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPlotVariablesDialog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "warningState.h"

#include "pqHoverLabel.h"
#include "pqPlotVariablesDialog.h"
#include "pqPlotter.h"
#include "pqSierraPlotToolsManager.h"

#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqSierraPlotToolsUtils.h"
#include "pqUndoStack.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolTip>
#include <QtDebug>

#include "ui_pqVariablePlot.h"

// used to show line number in #pragma message
#define STRING2(x) #x
#define STRING(x) STRING2(x)

///////////////////////////////////////////////////////////////////////////////
class pqPlotVariablesDialog::pqUI : public Ui::pqVariablePlot
{
};

//
// This class stores the range values for a variable and its
// components.
//
class VarRange
{
public:
  static const int MIN_ELEMENT_INDEX = 0;
  static const int MAX_ELEMENT_INDEX = 1;

  VarRange(QString name)
    : varName(name)
    , numComponents(0)
    , numElements(0)
    , ranges(NULL)
  {
  }

  virtual ~VarRange()
  {
    if (ranges)
    {
      for (int i = 0; i < numComponents; i++)
      {
        delete[] ranges[i];
      }
      delete[] ranges;

      ranges = 0;
    }

    if (magnitudes)
    {
      delete[] magnitudes;
    }
  }

  QString varName;
  int numComponents;
  int numElements;
  double** ranges;
  double* magnitudes;
};

//
// This class encapsulates the concept/implementation of the GUI elements
// that make up a "range".
// The "range" consists of labels for the min & max, line edit boxes for the
// min and max values
//
//
class pqRangeWidget
{
public:
  struct RangeWidgetGroup
  {
    RangeWidgetGroup(QLabel* minLabel, QLabel* maxLabel, QLineEdit* minLineEdit,
      QLineEdit* maxLineEdit, QFrame* minLabelEditFrame, QFrame* maxLabelEditFrame,
      QHBoxLayout* minLayout, QHBoxLayout* maxLayout)
    {
      this->minRangeLabel = minLabel;
      this->maxRangeLabel = maxLabel;
      this->minLineEditRange = minLineEdit;
      this->maxLineEditRange = maxLineEdit;
      this->minFrame = minLabelEditFrame;
      this->maxFrame = maxLabelEditFrame;
      this->minHorizontalLayout = minLayout;
      this->maxHorizontalLayout = maxLayout;
    }

    virtual ~RangeWidgetGroup()
    {
      delete this->minFrame;
      delete this->maxFrame;
    }

    QLabel* minRangeLabel;
    QLabel* maxRangeLabel;
    QLineEdit* minLineEditRange;
    QLineEdit* maxLineEditRange;
    QFrame* minFrame;
    QFrame* maxFrame;
    QHBoxLayout* minHorizontalLayout;
    QHBoxLayout* maxHorizontalLayout;
  };

  pqRangeWidget(QString variableAsString)
    : varName(variableAsString)
  {
  }

  virtual ~pqRangeWidget()
  {
    for (int i = 0; i < int(rangeWidgetGroups.size()); i++)
    {
      delete rangeWidgetGroups[i];
    }
    if (this->horizLine)
    {
      delete this->horizLine;
      this->horizLine = NULL;
    }
  }

  std::vector<RangeWidgetGroup*> rangeWidgetGroups;
  QFrame* horizLine;
  QString varName;

  static int precision;

  RangeWidgetGroup* allocAndMakeMinMax(VarRange* varRange, const QString& varToDisplay,
    int componentIndex, QVBoxLayout* theVarRangeLayout, QWidget* parentWidget)
  {
    QLabel* minRangeLabel;
    QLabel* maxRangeLabel;
    QLineEdit* maxLineEditRange;
    QLineEdit* minLineEditRange;

    QString minStr = varToDisplay + QString(" min");
    QString maxStr = varToDisplay + QString(" max");

    // Annotate variable min/max
    // varToDisplay Min, varToDisplay Max
    //

    QFrame* minFrame = new QFrame(parentWidget);
    minFrame->setMaximumSize(QSize(16777215, 40));
    QHBoxLayout* minHorizontalLayout = new QHBoxLayout(minFrame);
    minRangeLabel = new QLabel(minFrame);
    minRangeLabel->setObjectName(varRange->varName + QString("_") + QString("minRangeLabel"));
    minRangeLabel->setText(minStr);
    minHorizontalLayout->addWidget(minRangeLabel);
    minLineEditRange = new QLineEdit(minFrame);
    minLineEditRange->setObjectName(varRange->varName + QString("_") + QString("minLineEditRange"));

    double rangeVal = 0.0;
    if (componentIndex == -1)
    {
      // get the magnitude of the range
      rangeVal = varRange->magnitudes[VarRange::MIN_ELEMENT_INDEX];
    }
    else
    {
      // get range itself
      rangeVal = varRange->ranges[componentIndex][VarRange::MIN_ELEMENT_INDEX];
    }
    minLineEditRange->setText(QString("").setNum(rangeVal, 'e', pqRangeWidget::precision));
    minHorizontalLayout->addWidget(minLineEditRange);

    QFrame* maxFrame = new QFrame(parentWidget);
    maxFrame->setMaximumSize(QSize(16777215, 40));
    QHBoxLayout* maxHorizontalLayout = new QHBoxLayout(maxFrame);
    maxRangeLabel = new QLabel(maxFrame);
    maxRangeLabel->setObjectName(varRange->varName + QString("_") + QString("maxRangeLabel"));
    maxRangeLabel->setText(maxStr);
    maxHorizontalLayout->addWidget(maxRangeLabel);
    maxLineEditRange = new QLineEdit(maxFrame);
    maxLineEditRange->setObjectName(varRange->varName + QString("_") + QString("maxLineEditRange"));
    if (componentIndex == -1)
    {
      // get the magnitude of the range
      rangeVal = varRange->magnitudes[VarRange::MAX_ELEMENT_INDEX];
    }
    else
    {
      // get range itself
      rangeVal = varRange->ranges[componentIndex][VarRange::MAX_ELEMENT_INDEX];
    }
    maxLineEditRange->setText(QString("").setNum(rangeVal, 'e', pqRangeWidget::precision));
    maxHorizontalLayout->addWidget(maxLineEditRange);

    theVarRangeLayout->addWidget(minFrame);
    theVarRangeLayout->addWidget(maxFrame);

    RangeWidgetGroup* rangeWidgetGroup =
      new RangeWidgetGroup(minRangeLabel, maxRangeLabel, minLineEditRange, maxLineEditRange,
        minFrame, maxFrame, minHorizontalLayout, maxHorizontalLayout);

    return rangeWidgetGroup;
  }

  void build(pqPlotVariablesDialog::pqUI* ui, VarRange* varRange, int componentIndex)
  {
    rangeWidgetGroups.push_back(this->allocAndMakeMinMax(
      varRange, this->varName, componentIndex, ui->scrollWidgetLayout, ui->rangeScrollArea));

    // Add a (QFrame) horizontal line for visual separation
    horizLine = new QFrame(ui->rangeScrollArea);
    horizLine->setFrameShape(QFrame::HLine);
    ui->scrollWidgetLayout->addWidget(horizLine);
  }
};

//
// Storage for pqRangeWidget
//
int pqRangeWidget::precision = 0;

//
// Internal implementation for pqPlotVariablesDialog
//
class pqPlotVariablesDialog::pqInternal
{
public:
  static const int DEFAULT_FLOATING_POINT_PRECISION = 7;

  enum separators_enum
  {
    e_unknownSep = -1,
    e_commaSep = 0,
    e_dashSep,
  };

  pqInternal()
    : listWidget(NULL)
    , verticalSpacer(NULL)
    , plotType(-1)
  {
    varRanges.clear();
    rangeWidgets.clear();
    pqPlotVariablesDialog::pqInternal::precision =
      pqPlotVariablesDialog::pqInternal::DEFAULT_FLOATING_POINT_PRECISION;

    // we set the pqRangeWidget::precision class variable here, because we want the
    // DEFAULT_FLOATING_POINT_PRECISION
    //   to be associated with the pqPlotVariablesDialog::pqInternal, and not have to propagate it
    //   down
    //   to a "lower level" class
    pqRangeWidget::precision = pqPlotVariablesDialog::pqInternal::precision;

    validComponentSuffixes.append("_x");
    validComponentSuffixes.append("_y");
    validComponentSuffixes.append("_z");
    validComponentSuffixes.append("_xx");
    validComponentSuffixes.append("_xy");

    // symetric -- could be xz, or zx, but Exodus reader stores as 'zx'
    validComponentSuffixes.append("_zx");

    validComponentSuffixes.append("_yy");
    validComponentSuffixes.append("_yz");
    validComponentSuffixes.append("_zz");
    validComponentSuffixes.append("_magnitude");

    componentArrayIndicesMap["_magnitude"] = -1; // treat magnitude special!

    componentArrayIndicesMap["_x"] = 0;
    componentArrayIndicesMap["_y"] = 1;
    componentArrayIndicesMap["_z"] = 2;

    // The Symmetric Tensor – six components index order is defined
    // by the VTK Exodus reader
    //   see vtkExodusIIReaderPrivate.h
    //   and vtkExodusIIReader.cxx in VTK library
    componentArrayIndicesMap["_xx"] = 0;
    componentArrayIndicesMap["_yy"] = 1;
    componentArrayIndicesMap["_zz"] = 2;
    componentArrayIndicesMap["_xy"] = 3;
    componentArrayIndicesMap["_yz"] = 4;
    componentArrayIndicesMap["_zx"] = 5;
  }

  virtual ~pqInternal()
  {
    QMap<QString, VarRange*>::iterator mapI = varRanges.begin();
    while (mapI != varRanges.end())
    {
      VarRange* vr = mapI.value();
      delete vr;
      mapI++;
    }
  }

  virtual void addVariable(QString varName);
  virtual double computeMagnitude(VarRange* vr, int k);
  virtual bool addRangeToUI(pqPlotVariablesDialog::pqUI* ui, QString variableAsString);
  virtual bool removeRangeFromUI(pqPlotVariablesDialog::pqUI* ui, QString variableAsString);
  virtual bool inSelection(const QString& itemStr, QList<QListWidgetItem*>& selectedItems);

  virtual int getPlotType() { return plotType; }

  virtual void setPlotType(int type) { plotType = type; }

  virtual void setPlotter(pqPlotter* thePlotter) { plotter = thePlotter; }

  virtual pqPlotter* getPlotter() { return plotter; }

  //-----------------------------------------------------------------------------
  void updateHoverWithPlotter(pqPlotVariablesDialog::pqUI* _ui)
  {
    _ui->variableVsWhatever->setPlotter(this->getPlotter());
  }

  //-----------------------------------------------------------------------------
  int getNumberPostSeparator(int begIndex, QString lineEditText)
  {
    if (begIndex >= lineEditText.size())
    {
      return -1;
    }

    int index = begIndex;
    while (!isSeparator(lineEditText[index]) && (index < lineEditText.size()))
    {
      index++;
    }

    if (index >= lineEditText.size())
    {
      return lineEditText.size() - 1;
    }

    return index - 1;
  }

  //-----------------------------------------------------------------------------
  bool isSeparator(QChar ch)
  {
    if (ch.toLatin1() == ',')
    {
      return true;
    }
    if (ch.toLatin1() == '-')
    {
      return true;
    }

    return false;
  }

  //-----------------------------------------------------------------------------
  bool separator(QChar ch, separators_enum& sepType)
  {
    sepType = e_unknownSep;
    if (ch.toLatin1() == ',')
    {
      sepType = e_commaSep;
      return true;
    }
    if (ch.toLatin1() == '-')
    {
      sepType = e_dashSep;
      return true;
    }

    // qWarning() << "pqPlotVariablesDialog::pqInternal: * Error * Invalide range separator " << ;

    return false;
  }

  //-----------------------------------------------------------------------------
  int findSeparator(int parseIndex, separators_enum& sepType, QString lineEditText)
  {
    int i = parseIndex;
    sepType = e_unknownSep;
    while (!this->separator(lineEditText[i], sepType) && i < lineEditText.length())
    {
      i++;
    }

    if (i >= lineEditText.length())
    {
      return -1;
    }

    return i;
  }

  //-----------------------------------------------------------------------------
  // This method parses a string for a number.
  // if the starting parseIndex is a separator, this is an error
  //
  QPair<int, int> parseNumberRange(int& parseIndex, separators_enum& sepType, QString lineEditText)
  {
    sepType = e_unknownSep;

    QPair<int, int> retRange(-1, -1);
    int rangeBeg = -1;
    int rangeEnd = -1;

    if (lineEditText.length() <= 0)
    {
      return retRange;
    }

    if (isSeparator(lineEditText[parseIndex]))
    {
      return retRange;
    }

    if (parseIndex >= lineEditText.length())
    {
      return retRange;
    }

    // look for separator, starting at parseIndex
    int separatorIndex = this->findSeparator(parseIndex, sepType, lineEditText);

    if (separatorIndex == -1)
    {
      rangeBeg = this->util.getNumber(parseIndex, lineEditText.length() - 1, lineEditText);
      parseIndex = -1; // end of line or something
    }
    else
    {
      switch (sepType)
      {
        case e_commaSep:
          rangeBeg = this->util.getNumber(parseIndex, separatorIndex - 1, lineEditText);
          parseIndex = separatorIndex + 1;
          break;

        case e_dashSep:
        {
          rangeBeg = this->util.getNumber(parseIndex, separatorIndex - 1, lineEditText);
          int rangeEndIndex = this->getNumberPostSeparator(separatorIndex + 1, lineEditText);
          if (rangeEndIndex != -1)
          {
            rangeEnd = this->util.getNumber(separatorIndex + 1, rangeEndIndex, lineEditText);
            parseIndex = rangeEndIndex + 1;

            // if the next char is a separator, then advance the index to parse at
            if (isSeparator(lineEditText[parseIndex]))
            {
              parseIndex++;
            }
          }
          else
          {
            // error with range specification
            rangeBeg = -1;
            rangeEnd = -1;
          }
          break;
        }

        case e_unknownSep:
          // Do something here?
          break;
      }
    }

    retRange.first = rangeBeg;
    retRange.second = rangeEnd;

    return retRange;
  }

  //-----------------------------------------------------------------------------
  QString removeAllWhiteSpace_andValidate(QString inString, bool& errFlag)
  {
    errFlag = false;
    QString retString("");

    // pass 1 - remove all whitespace
    retString = this->util.removeAllWhiteSpace(inString);

    // pass 2 - look for invalid characters
    for (int i = 0; i < retString.length(); i++)
    {
      if (!this->util.validChar(retString[i].toLatin1()))
      {
        errFlag = true;
        break;
      }
    }

    // pass 3 -- set an error flag if the stripped string is empty
    if (retString.length() <= 0)
    {
      qWarning() << "removeAllWhiteSpace_andValidate: ERROR - selection string: " << inString
                 << ", is empty";
      errFlag = true;
    }

    return retString;
  }

  //-----------------------------------------------------------------------------
  QString componentSuffixString(QString variableAsString)
  {
    for (int i = 0; i < this->validComponentSuffixes.size(); i++)
    {
      if (variableAsString.endsWith(this->validComponentSuffixes[i]))
      {
        return this->validComponentSuffixes[i];
      }
    }

    return QString("");
  }

  //-----------------------------------------------------------------------------
  int componentArrayIndexFromSuffix(QString variableAsString)
  {
    int index = 0;

    QString suffixString = this->componentSuffixString(variableAsString);

    if (suffixString != QString(""))
    {
      index = this->componentArrayIndicesMap[suffixString];
    }

    return index;
  }

  //-----------------------------------------------------------------------------
  QString stripComponentSuffix(QString variableAsString)
  {
    QString retString = this->util.removeAllWhiteSpace(variableAsString);

    QString suffixString = componentSuffixString(retString);

    if (suffixString.size() > 0)
    {
      int positionToTruncate = retString.size() - suffixString.size();
      if (positionToTruncate > 0)
      {
        retString.truncate(positionToTruncate);
      }
    }

    return retString;
  }

  //=============================================================================

  QStringList validComponentSuffixes;
  QMap<QString, int> componentArrayIndicesMap;
  QMap<QString, VarRange*> varRanges;
  QMap<QString, bool> variableSelectionStates; // to keep track of selection state
  QVector<pqRangeWidget*> rangeWidgets;
  QListWidget* listWidget;
  QSpacerItem* verticalSpacer;
  pqSierraPlotToolsUtils util;

  static int precision;

  int plotType;
  pqPlotter* plotter;
};

int pqPlotVariablesDialog::pqInternal::precision =
  pqPlotVariablesDialog::pqInternal::DEFAULT_FLOATING_POINT_PRECISION;

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::pqInternal::addVariable(QString varName)
{
  VarRange* vr = this->varRanges[varName];
  if (vr == NULL)
  {
    vr = new VarRange(varName);
    this->varRanges[varName] = vr;
  }
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::pqInternal::removeRangeFromUI(
  pqPlotVariablesDialog::pqUI* _ui, QString variableAsString)
{
  pqRangeWidget* rangeWidget;

  int i;
  for (i = 0; i < this->rangeWidgets.size(); i++)
  {
    rangeWidget = this->rangeWidgets[i];
    if (rangeWidget->varName == variableAsString)
    {
      delete rangeWidget;
      this->rangeWidgets.remove(i);

      // if no more range widgets left, remove the vertical spacer
      if (this->rangeWidgets.size() == 0)
      {
        if (this->verticalSpacer)
        {
          _ui->scrollWidgetLayout->removeItem(this->verticalSpacer);
          this->verticalSpacer = NULL;
        }
      }

      // update the scroll area geometry
      // so that it resizes if need be
      _ui->rangeScrollArea->updateGeometry();

      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::pqInternal::addRangeToUI(
  pqPlotVariablesDialog::pqUI* _ui, QString variableAsString)
{
  QString strippedVariableName = this->stripComponentSuffix(variableAsString);
  int arrayIndex = this->componentArrayIndexFromSuffix(variableAsString);
  VarRange* varRange = this->varRanges[strippedVariableName];

  if (varRange != NULL)
  {
    pqRangeWidget* rangeWidget = new pqRangeWidget(variableAsString);
    rangeWidget->build(_ui, varRange, arrayIndex);

    this->rangeWidgets.append(rangeWidget);
  }
  else
  {
    qCritical() << "* ERROR * variable " << variableAsString << " has no valid range";
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
double pqPlotVariablesDialog::pqInternal::computeMagnitude(VarRange* vr, int k)
{
  double magnitude = 0.0;
  for (int i = 0; i < vr->numComponents; i++)
  {
    magnitude = magnitude + vr->ranges[i][k] * vr->ranges[i][k];
  }

  magnitude = sqrt(magnitude);

  return magnitude;
}
//=============================================================================
pqPlotVariablesDialog::pqPlotVariablesDialog(QWidget* p, Qt::WindowFlags f /*=0*/)
  : QDialog(p, f)
{
  pqSierraPlotToolsManager* manager = pqSierraPlotToolsManager::instance();
  this->Server = manager->getActiveServer();

  this->Internal = new pqPlotVariablesDialog::pqInternal();

  this->ui = new pqPlotVariablesDialog::pqUI;
  this->ui->setupUi(this);

  // connect signals and slots to the list widget
  QObject::connect(this->ui->okCancelButtonBox, SIGNAL(accepted(void)), this, SLOT(slotOk(void)));
  QObject::connect(
    this->ui->okCancelButtonBox, SIGNAL(rejected(void)), this, SLOT(slotCancel(void)));

  QObject::connect(this->ui->useParaViewGUIToSelectNodesCheckBox, SIGNAL(toggled(bool)), this,
    SLOT(slotUseParaViewGUIToSelectNodesCheckBox(bool)));

  // set up some parameters for the scroll area
  // scroll area height should not be more than 60% of desktop main screen
  this->ui->rangeScrollArea->setMaximumHeight(
    int(0.5 * QApplication::desktop()->availableGeometry().height()));

  // main dialog height should not be more than a certain percentage of desktop main screen

  // float deskTopHeight = QApplication::desktop()->availableGeometry().height();

  pqPlotVariablesDialog* myDialog = this;
  //  myDialog->setMaximumHeight(0.1 * deskTopHeight);

  myDialog->setMaximumHeight(555);

  // QSizePolicy tmpSizePolicy = this->sizePolicy(); //If this doesn't do anything, why is it here?
}

//-----------------------------------------------------------------------------
pqPlotVariablesDialog::~pqPlotVariablesDialog()
{
  delete this->ui;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QSize pqPlotVariablesDialog::sizeHint() const
{
  QSize dialogSizeHint = this->QDialog::sizeHint();

  float deskTopHeight = QApplication::desktop()->availableGeometry().height();

  dialogSizeHint.setHeight(int(0.1 * deskTopHeight));

  return dialogSizeHint;
}

//-----------------------------------------------------------------------------
QString pqPlotVariablesDialog::stripComponentSuffix(QString variableAsString)
{
  return this->Internal->stripComponentSuffix(variableAsString);
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::activateSelectionByNumberFrame()
{
  if (this->Internal->getPlotter()->amIAbleToSelectByNumber())
  {
    this->ui->selectionByNumberFrame->show();
    this->setupActivationForOKButton(true);
  }
  else
  {
    this->ui->selectionByNumberFrame->hide();
    this->setupActivationForOKButton(false);
  }
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setNumberItemsLabel(QString value)
{
  this->ui->numberItemsLabel->setText(value);
}

//-----------------------------------------------------------------------------
int pqPlotVariablesDialog::getPlotType()
{
  return this->Internal->getPlotType();
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setPlotType(int type)
{
  this->Internal->setPlotType(type);
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setPlotter(pqPlotter* thePlotter)
{
  this->Internal->setPlotter(thePlotter);

  this->Internal->updateHoverWithPlotter(this->ui);
}

//-----------------------------------------------------------------------------
pqPlotter* pqPlotVariablesDialog::getPlotter()
{
  return this->Internal->getPlotter();
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::addVariable(QString varName)
{
  this->Internal->addVariable(varName);
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setFloatingPointPrecision(int precis)
{
  pqPlotVariablesDialog::pqInternal::precision = precis;

  // we set the pqRangeWidget::precision class variable here, because we want the concept of
  // "precision"
  //   to be associated with the pqPlotVariablesDialog::pqInternal, and not have to propagate it
  //   down
  //   to a "lower level" class
  pqRangeWidget::precision = pqPlotVariablesDialog::pqInternal::precision;
}

//-----------------------------------------------------------------------------
int pqPlotVariablesDialog::getFloatingPointPrecision()
{
  return pqRangeWidget::precision;
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::allocSetRange(
  QString varName, int numComp, int numElems, double** ranges)
{
  VarRange* vr = this->Internal->varRanges[varName];
  if (vr != NULL)
  {
    vr->numComponents = numComp;
    vr->numElements = numElems;

    vr->ranges = new double*[numComp];
    for (int j = 0; j < numComp; j++)
    {
      vr->ranges[j] = new double[numElems];
      for (int k = 0; k < numElems; k++)
      {
        vr->ranges[j][k] = ranges[j][k];
      }
    }

    // compute the magnitude
    vr->magnitudes = new double[numElems];
    for (int k = 0; k < numElems; k++)
    {
      vr->magnitudes[k] = this->Internal->computeMagnitude(vr, k);
    }
  }
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::getUseParaViewGUIToSelectNodesCheckBoxState()
{
  return this->ui->useParaViewGUIToSelectNodesCheckBox->isChecked();
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setEnableNumberItems(bool flag)
{
  this->ui->numberItemsLabel->setEnabled(flag);
  this->ui->numberItemsLineEdit->setEnabled(flag);
  this->setupActivationForOKButton(flag);
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setupActivationForOKButton(bool flag)
{
  if (flag)
  {
    // Disable the OK button, until some range is entered in the numberItemsLineEdit box
    QPushButton* okButton = this->ui->okCancelButtonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    // connect a signal for when the user enters some data in numberItemsLineEdit box
    connect(this->ui->numberItemsLineEdit, SIGNAL(textChanged(const QString&)), this,
      SLOT(slotTextChanged(const QString&)));
  }
  else
  {
    // Enable the OK button
    QPushButton* okButton = this->ui->okCancelButtonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::slotTextChanged(const QString& text)
{
  QString nonWhite = this->Internal->util.removeAllWhiteSpace(text);
  QPushButton* okButton = this->ui->okCancelButtonBox->button(QDialogButtonBox::Ok);
  if (nonWhite.size() > 0)
  {
    okButton->setEnabled(true);
  }
  else
  {
    okButton->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
QString pqPlotVariablesDialog::getNumberItemsLineEdit()
{
  return this->ui->numberItemsLineEdit->text();
}

//-----------------------------------------------------------------------------
QList<int> pqPlotVariablesDialog::determineSelectedItemsList(bool& errFlag)
{
  QList<int> retList;

  QString lineEditText =
    this->Internal->removeAllWhiteSpace_andValidate(this->ui->numberItemsLineEdit->text(), errFlag);

  if (errFlag)
  {
    return retList;
  }

  int parseIndex = 0;
  while (parseIndex != -1 && parseIndex < lineEditText.length())
  {
    QPair<int, int> range;

    pqInternal::separators_enum sepType;
    range = this->Internal->parseNumberRange(parseIndex, sepType, lineEditText);

    if (range.first >= 0)
    {
      if (sepType == pqInternal::e_dashSep)
      {
        // if dash, find the range, if comma (or something else god forbid!) then don't do the range
        int i;
        for (i = range.first; i <= range.second; i++)
        {
          // only append unique values
          if (!retList.contains(i))
          {
            retList.append(i);
          }
        }
      }
      else
      {
        // comma, so just put in the first value
        retList.append(range.first);
      }
    }
    else
    {
      // some sort of error, deal with it
      qWarning()
        << "pqPlotVariablesDialog::determineSelectedItemsList: ERROR - range specification error";
      errFlag = true;
      return retList;
    }
  }

  return retList;
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::slotUseParaViewGUIToSelectNodesCheckBox(bool /*checked*/)
{
  emit this->useParaViewGUIToSelectNodesCheck();
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::slotOk(void)
{
  // user clicked on OK button
  emit this->okDismissed();
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::slotCancel(void)
{
  // user clicked on OK button
  emit this->cancelDismissed();
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::areVariablesSelected()
{
  return (this->Internal->listWidget->selectedItems().count() > 0);
}

//-----------------------------------------------------------------------------
QList<QListWidgetItem*> pqPlotVariablesDialog::getSelectedItems()
{
  return this->Internal->listWidget->selectedItems();
}

//-----------------------------------------------------------------------------
QStringList pqPlotVariablesDialog::getVarsWithComponentSuffixes(
  vtkSMStringVectorProperty* stringVecProp)
{
  QStringList retList;

  unsigned int uNumElems = 0;
  uNumElems = stringVecProp->GetNumberOfElements();

  for (unsigned int i = 0; i < uNumElems; i += 2)
  {
    const char* elemPtr = stringVecProp->GetElement(i);
    // const char * elemPtr_status = stringVecProp->GetElement(i+1);

    QString elemAsQString(elemPtr);
    VarRange* vr = this->Internal->varRanges[elemAsQString];
    if (vr != NULL)
    {
      QStringList compList;

      switch (vr->numComponents)
      {
        case 1:
          compList.append(elemAsQString);
          break;

        case 3:
          compList.append(elemAsQString + QString("_x"));
          compList.append(elemAsQString + QString("_y"));
          compList.append(elemAsQString + QString("_z"));
          compList.append(elemAsQString + QString("_magnitude"));
          break;

        case 6:
          compList.append(elemAsQString + QString("_xx"));
          compList.append(elemAsQString + QString("_yy"));
          compList.append(elemAsQString + QString("_zz"));
          compList.append(elemAsQString + QString("_xy"));
          compList.append(elemAsQString + QString("_yz"));
          compList.append(elemAsQString + QString("_zx"));
          compList.append(elemAsQString + QString("_magnitude"));
          break;

        default:
          break;
      }

      retList.append(compList);
    }
  }

  return retList;
}

//-----------------------------------------------------------------------------
QStringList pqPlotVariablesDialog::getSelectedItemsStringList()
{
  QList<QListWidgetItem*> selectedItems = this->getSelectedItems();
  QStringList retList;

  QList<QListWidgetItem*>::iterator iter = selectedItems.begin();
  while (iter != selectedItems.end())
  {
    QString variableAsString = (*iter)->text();
    retList.append(variableAsString);
    iter++;
  }

  return retList;
}

//-----------------------------------------------------------------------------
QListWidget* pqPlotVariablesDialog::getVariableList()
{
  return this->Internal->listWidget;
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setupVariablesList(QStringList varStrings)
{
  QGridLayout* varListLayout = new QGridLayout(ui->pickVariableFrame);
  this->Internal->listWidget = new QListWidget(ui->pickVariableFrame);
  varListLayout->addWidget(this->Internal->listWidget);

  this->Internal->listWidget->setSelectionMode(QAbstractItemView::MultiSelection);

  QStringList::const_iterator constIterator;
  for (constIterator = varStrings.constBegin(); constIterator != varStrings.constEnd();
       ++constIterator)
  {
    QString theVarNameStr = (*constIterator);
    this->Internal->listWidget->addItem(theVarNameStr);
    this->Internal->variableSelectionStates[theVarNameStr] = false;
  }

  // connect signals and slots to the list widget
  QObject::connect(this->Internal->listWidget, SIGNAL(itemSelectionChanged()), this,
    SLOT(slotItemSelectionChanged()));
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::pqInternal::inSelection(
  const QString& itemStr, QList<QListWidgetItem*>& selectedItems)
{
  QList<QListWidgetItem*>::iterator iter;
  for (iter = selectedItems.begin(); iter != selectedItems.end(); iter++)
  {
    if ((*iter)->text() == itemStr)
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::slotItemSelectionChanged()
{
  QList<QListWidgetItem*> selectedItems = this->Internal->listWidget->selectedItems();

  // Find all deselected variables:
  // first go through all the selection states. If a variable was selected,
  // but is not in the current selection list, than that means it was
  // delselected
  QMap<QString, bool>::iterator statesIter = this->Internal->variableSelectionStates.begin();
  for (; statesIter != this->Internal->variableSelectionStates.end(); statesIter++)
  {
    if (*statesIter == true && !this->Internal->inSelection(statesIter.key(), selectedItems))
    {
      // deselection
      emit this->variableDeselectionByName(statesIter.key());

      // and set the selection state accordingly
      this->Internal->variableSelectionStates[statesIter.key()] = false;
    }
  }

  // Now find all selected variables (that were not selected in the first pass):
  // first go through all the selection states. If a variable was selected,
  // but is not in the current selection list, than that means it was
  // delselected
  statesIter = this->Internal->variableSelectionStates.begin();
  for (; statesIter != this->Internal->variableSelectionStates.end(); statesIter++)
  {
    if (*statesIter == false && this->Internal->inSelection(statesIter.key(), selectedItems))
    {
      // new selection
      emit this->variableSelectionByName(statesIter.key());

      // and set the selection state accordingly
      this->Internal->variableSelectionStates[statesIter.key()] = true;
    }
  }

  // pqPlotVariablesDialog * myDialog = this;

  // QSize dialogSize = myDialog->size();
  // QSize maxSize = myDialog->maximumSize();
  // QSizePolicy tmpSizePolicy = myDialog->sizePolicy();
  // QSizePolicy::Policy verticalPolicy = sizePolicy.verticalPolicy();
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::addRangeToUI(QString itemText)
{

  // Remove the verticalSpacer (if it exists)
  // because we always want to add it after the last range
  if (this->Internal->verticalSpacer)
  {
    ui->scrollWidgetLayout->removeItem(this->Internal->verticalSpacer);
    this->Internal->verticalSpacer = NULL;
  }

  bool flag = this->Internal->addRangeToUI(this->ui, itemText);

  if (!flag)
  {
    return false;
  }

  // add in a vertical spacer
  this->Internal->verticalSpacer =
    new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  this->ui->scrollWidgetLayout->addItem(this->Internal->verticalSpacer);

  // update the scroll area geometry
  // so that it resizes if need be
  this->ui->rangeScrollArea->updateGeometry();

  return true;
}

//-----------------------------------------------------------------------------
bool pqPlotVariablesDialog::removeRangeFromUI(QString itemText)
{
  return this->Internal->removeRangeFromUI(this->ui, itemText);
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setHeading(QString heading)
{
  this->ui->variableVsWhatever->setText(heading);
}

//-----------------------------------------------------------------------------
void pqPlotVariablesDialog::setTimeRange(double min, double max)
{
  QString str;

  str = QString("%1").arg(min, 0, 'E');
  this->ui->timeMinLineEdit->setText(str);
  str = QString("%1").arg(max, 0, 'E');
  this->ui->timeMaxLineEdit->setText(str);
}
