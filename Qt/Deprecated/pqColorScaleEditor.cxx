/*=========================================================================

   Program: ParaView
   Module:    pqColorScaleEditor.cxx

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

=========================================================================*/

/// \file pqColorScaleEditor.cxx
/// \date 2/14/2007

#include "pqColorScaleEditor.h"
#include "ui_pqColorScaleDialog.h"

#include "pqApplicationCore.h"
#include "pqChartValue.h"
#include "pqColorMapModel.h"
#include "pqColorPresetManager.h"
#include "pqColorPresetModel.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqLookupTableManager.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewBase.h"
#include "pqRescaleRange.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarOpacityFunction.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqStandardColorLinkAdaptor.h"
#include "pqTransferFunctionChartViewWidget.h"
#include "vtkAbstractArray.h"
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkColor.h"
#include "vtkColorTransferControlPointsItem.h"
#include "vtkCompositeControlPointsItem.h"
#include "vtkPiecewiseControlPointsItem.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkColorTransferFunction.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMath.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkType.h"
#include "vtkSmartPointer.h"

#include <QCloseEvent>
#include <QColor>
#include <QColorDialog>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QIntValidator>
#include <QItemSelectionModel>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QSpacerItem>
#include <QString>
#include <QtDebug>
#include <QVariant>
#include "QVTKWidget.h"

#include <set>
#include <sstream>

#define PQ_INTERPRET_INTERVAL 0
#define PQ_INTERPRET_CATEGORY 1

#define PQ_COLORS_PAGE 0
#define PQ_ANNOTATION_PAGE 1
#define PQ_LEGEND_PAGE 2

#define PQ_ANN_VALUE_COL 0
#define PQ_ANN_ENTRY_COL 1

// A simple subclass of QTreeWidgetItem that overrides the default sorting behavior.
// If both items being compared can be converted to numbers, they are compared as numbers.
// Otherwise, string comparison is used.
class pqAnnotationTreeItem : public QTreeWidgetItem
{
public:
  pqAnnotationTreeItem(QTreeWidget* parnt) : QTreeWidgetItem(parnt) { }
protected:
  virtual bool operator < ( const QTreeWidgetItem& other ) const
    {
    int column = this->treeWidget()->sortColumn();
    vtkVariant x( this->data( column, Qt::DisplayRole ).toString().toAscii().data() );
    vtkVariant y( other.data( column, Qt::DisplayRole ).toString().toAscii().data() );
    double dx, dy;
    bool vx, vy;
    dx = x.ToDouble( &vx );
    dy = y.ToDouble( &vy );
    if ( vx && vy )
      {
      return dx < dy;
      }
    return x < y;
    }
};

class pqColorScaleEditorForm : public Ui::pqColorScaleDialog
{
public:
  pqColorScaleEditorForm();
  ~pqColorScaleEditorForm() { }

  void updateInterpretation( bool indexedLookup );

  pqPropertyLinks Links; // used to link properties on the legend
  pqPropertyLinks ReprLinks; // used to link properties on the representation.
  pqSignalAdaptorColor *TitleColorAdaptor;
  pqSignalAdaptorColor *LabelColorAdaptor;
  pqStandardColorLinkAdaptor* TitleColorLink;
  pqStandardColorLinkAdaptor* LabelColorLink;
  pqSignalAdaptorComboBox *TitleFontAdaptor;
  pqSignalAdaptorComboBox *LabelFontAdaptor;
  vtkEventQtSlotConnect *Listener;
  pqColorPresetManager *Presets;
  QButtonGroup* Interpretation;
  bool InSetColors;
  // Is the vtkScalarsToColors SM proxy's "Annotations" property being updated via this class (the GUI)?
  // Or, is the property being changed by the server or another client?
  // If TRUE, we should not try to update the GUI with the new values. If FALSE, we should update the GUI to match.
  bool InSetAnnotation;
  bool InSetInterpretation;
  bool IgnoreEditor;
  bool MakingLegend;
  vtkSmartPointer<vtkEventQtSlotConnect> ColorFunctionConnect;
  vtkSmartPointer<vtkEventQtSlotConnect> OpacityFunctionConnect;
};

void pqColorScaleEditorForm::updateInterpretation( bool indexedLookup )
{
  this->Interpretation->blockSignals( true );
  this->IntervalValues->setChecked( ! indexedLookup );
  this->CategoricalValues->setChecked( indexedLookup );
  this->Interpretation->blockSignals( false );
  this->ColorTabs->setTabEnabled( PQ_COLORS_PAGE, ! indexedLookup );
  this->UseLogScaleSimple->setDisabled( indexedLookup );
  this->UseAutoRescaleSimple->setDisabled( indexedLookup );
  this->SimpleMin->setDisabled( indexedLookup ? true : (this->UseAutoRescale->isChecked()));
  this->SimpleMax->setDisabled( indexedLookup ? true : (this->UseAutoRescale->isChecked()));

  if ( indexedLookup )
    {
    if ( this->ColorTabs->currentIndex() == PQ_COLORS_PAGE )
      {
      this->ColorTabs->setCurrentIndex( PQ_ANNOTATION_PAGE );
      }
    }
  else
    {
    if ( this->ColorTabs->currentIndex() == PQ_ANNOTATION_PAGE )
      {
      this->ColorTabs->setCurrentIndex( PQ_COLORS_PAGE );
      }
    }
}


//----------------------------------------------------------------------------
pqColorScaleEditorForm::pqColorScaleEditorForm()
  : Ui::pqColorScaleDialog(), Links()
{
  this->TitleColorAdaptor = 0;
  this->LabelColorAdaptor = 0;
  this->TitleColorLink = 0;
  this->LabelColorLink = 0;
  this->TitleFontAdaptor = 0;
  this->LabelFontAdaptor = 0;
  this->Listener = 0;
  this->Presets = 0;
  this->InSetColors = false;
  this->InSetAnnotation = false;
  this->InSetInterpretation = false;
  this->IgnoreEditor = false;
  this->MakingLegend = false;
  this->Interpretation = new QButtonGroup;
}

//----------------------------------------------------------------------------
pqColorScaleEditor::pqColorScaleEditor(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqColorScaleEditorForm();
  this->Display = 0;
  this->ColorMap = 0;
  this->OpacityFunction = 0;
  this->Legend = 0;
  this->ActiveUniqueValues = 0;

  // Set up the ui.
  this->Form->setupUi(this);
  this->Form->Listener = vtkEventQtSlotConnect::New();
  this->Form->Presets = new pqColorPresetManager(this);
  this->Form->Presets->setUsingCloseButton(true);
  this->Form->Presets->restoreSettings();
  this->connect(this->Form->Presets->getSelectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(applyPreset()));

  // Put the interval and catorical data interpretation buttons in the "interpretation" button group:
  this->Form->Interpretation->addButton( this->Form->IntervalValues );
  this->Form->Interpretation->addButton( this->Form->CategoricalValues );
  this->Form->Interpretation->setId(    this->Form->IntervalValues, PQ_INTERPRET_INTERVAL );
  this->Form->Interpretation->setId( this->Form->CategoricalValues, PQ_INTERPRET_CATEGORY );
  // Colormap domain value interpretation: interval/ratio or categorical/nominal?
  this->connect( this->Form->Interpretation, SIGNAL(buttonClicked(int)), this, SLOT(setInterpretation(int)) );

  // Annotation
  this->Form->AnnotationTree->setDragDropOverwriteMode( false );
  this->Form->AnnotationTree->setDragEnabled( true );
  this->Form->AnnotationTree->setDragDropMode( QAbstractItemView::InternalMove );
  this->Form->AnnotationTree->sortByColumn( -1, Qt::DescendingOrder );
  QObject::connect(
    this->Form->AnnotationTree->header(), SIGNAL(sectionClicked(int)),
    this, SLOT(updateAnnotationColors()));
  //this->Form->AnnotationTree->setSortingEnabled( false );
  //this->Form->AnnotationTree->verticalHeader()->setMovable( true );
  QObject::connect(this->Form->AnnotationTree, SIGNAL(itemSelectionChanged()),
    this, SLOT(annotationSelectionChanged()));
  this->Form->AnnotationTree->viewport()->installEventFilter( this );
  QObject::connect(
    this->Form->AnnotationTree->model(), SIGNAL(layoutChanged()),
    this, SLOT(annotationsChanged()) );
  QObject::connect(
    this->Form->AnnotationTree->model(), SIGNAL(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)),
    this, SLOT(annotationsChanged()) );
  QObject::connect( this->Form->RemoveAnnotation, SIGNAL(clicked()), this, SLOT(removeAnnotation()) );
  QObject::connect( this->Form->ResetAnnotations, SIGNAL(clicked()), this, SLOT(resetAnnotations()) );
  QObject::connect( this->Form->AddActiveValues,  SIGNAL(clicked()), this, SLOT(addActiveValues()) );
  QObject::connect( this->Form->NewAnnotation,    SIGNAL(clicked()), this, SLOT(addAnnotationEntry()) );

  // Force a redraw immediately when the "Render View Immediately" button gets checked...
  // Some changes may already have been made.
  this->connect(this->Form->checkBoxImmediateRender, SIGNAL(clicked()),
    this, SLOT(renderViewOptionally()));

  // Color transfer function widgets
  this->restoreOptionalUserSettings();
  this->Form->ScalarColor->setVisible(0);
  this->Form->pushButtonScalarColor->setFixedHeight(40);
  this->Form->pushButtonScalarColor->setText("");
  this->Form->pushButtonScalarColor->setVisible(0);
  this->connect(this->Form->pushButtonScalarColor,SIGNAL(clicked()),
    this->Form->ScalarColor, SLOT(chooseColor()));

  this->connect(this->Form->ScalarColor,SIGNAL(chosenColorChanged(const QColor &)),
    this, SLOT(setScalarColor(const QColor &)));
  this->connect(this->Form->opacityScalar, SIGNAL(editingFinished()),
    this, SLOT(setOpacityScalarFromText()));
  this->connect(this->Form->pushButtonApply, SIGNAL(clicked()),
    this, SLOT(updateDisplay()));

  this->UseEnableOpacityCheckBox = false;
  this->connect(this->Form->EnableOpacityFunction, SIGNAL(stateChanged(int)),
    this, SLOT(setEnableOpacityMapping(int)));

  QLayout* tfLayout = this->Form->frameColorTF->layout();
  this->ColorMapViewer = new pqTransferFunctionChartViewWidget(this);
  this->ColorMapViewer->setFixedHeight(40);
  this->OpacityFunctionViewer = new pqTransferFunctionChartViewWidget(this);
  this->OpacityFunctionViewer->setSizePolicy(
    QSizePolicy::Expanding, QSizePolicy::Expanding);
  this->OpacityFunctionViewer->setMinimumHeight(60);
  tfLayout->addWidget(this->ColorMapViewer);

  QVBoxLayout* opacityLayout = new QVBoxLayout(this->Form->frameOpacity);
  opacityLayout->setMargin(0);
  opacityLayout->addWidget(this->OpacityFunctionViewer);
  this->Form->frameOpacity->setVisible(0);

  this->Form->ColorFunctionConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Form->OpacityFunctionConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  double validBounds[4] = {VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, 0, 1};
  this->OpacityFunctionViewer->setValidBounds(validBounds);
  this->ColorMapViewer->setValidBounds(validBounds);

  this->OpacityFunctionViewer->addCompositeFunction(0, 0, true, true);
  vtkCompositeControlPointsItem* composite =
    vtkCompositeControlPointsItem::SafeDownCast(
    this->OpacityFunctionViewer->opacityFunctionPlots()[1]);
  composite->SetColorFill(true);
  composite->SetPointsFunction(vtkCompositeControlPointsItem::OpacityPointsFunction);
  this->ColorMapViewer->addColorTransferFunction(0);

  // Initialize the state of some of the controls.
  this->enableRescaleControls(this->Form->UseAutoRescale->isChecked());
  this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());

  this->enableLegendControls(this->Form->ShowColorLegend->isChecked());

  // Add the color space options to the combo box.
  this->Form->ColorSpace->addItem("RGB");
  this->Form->ColorSpace->addItem("HSV");
  this->Form->ColorSpace->addItem("Wrapped HSV");
  this->Form->ColorSpace->addItem("CIELAB");
  this->Form->ColorSpace->addItem("Diverging");

  // Add the color scale presets menu.
  this->loadBuiltinColorPresets();

  // Make sure the line edits only allow number inputs.
  this->Form->ScalarValue->setValidator(new QDoubleValidator(this));
  QDoubleValidator* opacityValid = new QDoubleValidator(this);
  opacityValid->setRange(0.0, 1.0);
  opacityValid->setDecimals(6);
  this->Form->Opacity->setValidator(opacityValid);
  this->Form->opacityScalar->setValidator(new QDoubleValidator(this));
  this->Form->ScalarOpacityUnitDistance->setValidator(
    new QDoubleValidator(this));

  QIntValidator *intValidator = new QIntValidator(this);
  this->Form->TableSizeText->setValidator(intValidator);

  // Connect the color scale widgets.
  this->connect(this->Form->ScalarValue, SIGNAL(editingFinished()),
      this, SLOT(setScalarFromText()));
  this->connect(this->Form->Opacity, SIGNAL(editingFinished()),
      this, SLOT(setOpacityFromText()));

  this->connect(this->Form->ColorSpace, SIGNAL(currentIndexChanged(int)),
      this, SLOT(setColorSpace(int)));

  this->connect(this->Form->NanColor,SIGNAL(chosenColorChanged(const QColor &)),
                this, SLOT(setNanColor(const QColor &)));
  this->connect(this->Form->NanColor2,SIGNAL(chosenColorChanged(const QColor &)),
                this, SLOT(setNanColor2(const QColor &)));
  this->connect(this->Form->AnnotationSwatch,SIGNAL(chosenColorChanged(const QColor &)),
                this, SLOT(editAnnotationColor(const QColor &)));

  this->connect(this->Form->SaveButton, SIGNAL(clicked()),
      this, SLOT(savePreset()));
  this->connect(this->Form->PresetButton, SIGNAL(clicked()),
      this, SLOT(loadPreset()));

  this->connect(this->Form->UseLogScale, SIGNAL(toggled(bool)),
      this, SLOT(setLogScale(bool)));

  this->connect(this->Form->UseAutoRescale, SIGNAL(toggled(bool)),
      this, SLOT(setAutoRescale(bool)));
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()),
      this, SLOT(rescaleToNewRange()));
  this->connect(this->Form->RescaleToDataButton, SIGNAL(clicked()),
      this, SLOT(rescaleToDataRange()));
  this->connect(this->Form->RescaleToDataOverTimeButton, SIGNAL(clicked()),
      this, SLOT(rescaleToDataRangeOverTime()));

  this->connect(this->Form->UseDiscreteColors, SIGNAL(toggled(bool)),
      this, SLOT(setUseDiscreteColors(bool)));
  this->connect(this->Form->TableSize, SIGNAL(valueChanged(int)),
      this, SLOT(setSizeFromSlider(int)));
  this->connect(this->Form->TableSizeText, SIGNAL(editingFinished()),
      this, SLOT(setSizeFromText()));

  // Connect the color legend widgets.
  this->connect(this->Form->ShowColorLegend, SIGNAL(toggled(bool)),
      this, SLOT(setLegendVisibility(bool)));

  this->connect(this->Form->TitleName, SIGNAL(textChanged(const QString &)),
      this, SLOT(setLegendName(const QString &)));
  this->connect(this->Form->TitleComponent, SIGNAL(textChanged(const QString &)),
      this, SLOT(setLegendComponent(const QString &)));
  this->Form->TitleColorAdaptor = new pqSignalAdaptorColor(
      this->Form->TitleColorButton, "chosenColor",
      SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Form->TitleFontAdaptor = new pqSignalAdaptorComboBox(
      this->Form->TitleFont);

  this->Form->LabelColorAdaptor = new pqSignalAdaptorColor(
      this->Form->LabelColorButton, "chosenColor",
      SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Form->LabelFontAdaptor = new pqSignalAdaptorComboBox(
      this->Form->LabelFont);

  // Hook the close button up to the accept action.
  this->connect(this->Form->CloseButton, SIGNAL(clicked()),
      this, SLOT(accept()));

  //Hook up the MakeDefaultButton
  this->connect(this->Form->MakeDefaultButton, SIGNAL(clicked()),
      this, SLOT(makeDefault()));

  // =========================
  // Simple UI add-on
  // =========================
  this->connect( this->Form->AdvancedButton, SIGNAL(toggled(bool)),
                 this, SLOT(enableAdvancedPanel(bool)));
  this->enableAdvancedPanel(this->Form->AdvancedButton->isChecked());

  this->connect( this->Form->UseLogScaleSimple, SIGNAL(toggled(bool)),
                 this, SLOT(setLogScale(bool)));

  this->connect( this->Form->UseAutoRescaleSimple, SIGNAL(toggled(bool)),
                 this, SLOT(setAutoRescale(bool)));

  this->connect( this->Form->SimpleMin, SIGNAL(editingFinished()),
                 this, SLOT(rescaleToSimpleRange()));

  this->connect( this->Form->SimpleMax, SIGNAL(editingFinished()),
                 this, SLOT(rescaleToSimpleRange()));

  this->connect( this->Form->AnnotationTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
                 this, SLOT(annotationsChanged()) );

  // Make sure the line edits only allow number inputs.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Form->SimpleMin->setValidator(validator);
  this->Form->SimpleMax->setValidator(validator);
}

pqColorScaleEditor::~pqColorScaleEditor()
{
  // Save the color map presets.
  this->Form->Presets->saveSettings();
  this->saveOptionalUserSettings();

  if ( this->ActiveUniqueValues )
    this->ActiveUniqueValues->Delete();

  delete this->Form->LabelColorAdaptor;
  delete this->Form->TitleColorAdaptor;
  delete this->Form->LabelFontAdaptor;
  delete this->Form->TitleFontAdaptor;
  this->Form->Listener->Delete();
  delete this->Form;
}

void pqColorScaleEditor::setRepresentation(pqDataRepresentation *display)
{
  if(this->Display == display)
    {
    return;
    }

  this->setLegend(0);
  this->Form->ShowColorLegend->setEnabled(false);
  if(this->Display)
    {
    this->disconnect(this->Display, 0, this, 0);
    this->disconnect(&this->Form->Links, 0, this->Display, 0);
    this->disconnect(&this->Form->ReprLinks, 0, this->Display, 0);
    this->Form->ReprLinks.removeAllPropertyLinks();
    if(this->ColorMap)
      {
      this->disconnect(this->ColorMap, 0, this, 0);
      this->Form->Listener->Disconnect(
        this->ColorMap->getProxy()->GetProperty("RGBPoints"));
      this->Form->Listener->Disconnect(
        this->ColorMap->getProxy()->GetProperty("EnableOpacityMapping"));
      this->Form->Listener->Disconnect(
        this->ColorMap->getProxy()->GetProperty( "Annotations" ) );
      }

    if(this->OpacityFunction)
      {
      this->Form->Listener->Disconnect(
          this->OpacityFunction->getProxy()->GetProperty("Points"));
      }
    }

  this->Display = display;
  this->ColorMap = 0;
  this->OpacityFunction = 0;
  if(this->Display)
    {
    this->connect(this->Display, SIGNAL(destroyed(QObject *)),
        this, SLOT(cleanupDisplay()));
    this->connect(&this->Form->Links, SIGNAL(qtWidgetChanged()),
        this, SLOT(renderViewOptionally()));
    this->connect(&this->Form->ReprLinks, SIGNAL(qtWidgetChanged()),
        this, SLOT(renderViewOptionally()));

    // Get the color map object for the display's lookup table.
    this->ColorMap = this->Display->getLookupTable();
    if(this->ColorMap)
      {
      this->connect(this->ColorMap, SIGNAL(destroyed(QObject *)),
          this, SLOT(cleanupDisplay()));
      this->connect(this->ColorMap, SIGNAL(scalarBarsChanged()),
          this, SLOT(checkForLegend()));
      this->Form->Listener->Connect(
          this->ColorMap->getProxy()->GetProperty("RGBPoints"),
          vtkCommand::ModifiedEvent, this, SLOT(handleColorPointsChanged()));
      this->Form->Listener->Connect(
          this->ColorMap->getProxy()->GetProperty("EnableOpacityMapping"),
          vtkCommand::ModifiedEvent, this, SLOT(handleEnableOpacityMappingChanged()));
      this->Form->Listener->Connect(
        this->ColorMap->getProxy()->GetProperty( "IndexedLookup" ),
        vtkCommand::ModifiedEvent, this, SLOT(handleInterpretationChanged()) );
      this->Form->Listener->Connect(
        this->ColorMap->getProxy()->GetProperty( "Annotations" ),
        vtkCommand::ModifiedEvent, this, SLOT(handleAnnotationsChanged()) );
      this->handleInterpretationChanged();
      this->handleAnnotationsChanged();
      //bool indexMode = this->ColorMap->getIndexedLookup();
      //this->Form->updateInterpretation( indexMode );
      }

    int acomp = ( display->getLookupTable()->getVectorMode() == pqScalarsToColors::MAGNITUDE ? -1 :
      display->getLookupTable()->getVectorComponent() );
    vtkPVProminentValuesInformation* vinfo = display->getProxyColorProminentValuesInfo();
    bool haveActiveValues = false;
    if ( vinfo )
      {
      if ( acomp == -1 && vinfo->GetNumberOfComponents() == 1 )
        {
        acomp = 0;
        }
      vtkAbstractArray* uniq = vinfo->GetProminentComponentValues(acomp);
      if ( uniq )
        {
        haveActiveValues = true;
        // If the lookup table doesn't have any entries and the active representation has a small set, populate the lookup table
        // with these values by default. Otherwise, let the user add any new values with the "Add ## active values" button.
        this->setActiveUniqueValues( uniq );
        //if ( this->ColorMap->GetNumberOfAnnotations() == 0 )
        if ( this->Form->AnnotationTree->topLevelItemCount() == 0 )
          {
          this->addActiveValues();
          }
        uniq->Delete();
        }
      }
    if ( ! haveActiveValues )
      {
      this->Form->AddActiveValues->setText( "Too Many/Few Values" );
      this->Form->AddActiveValues->setEnabled( false );
      }
    }

  // Disable the gui elements if the color map is null.
  this->Form->ColorTabs->setEnabled(this->ColorMap != 0);

  this->initColorScale();
  if(this->ColorMap)
    {
    pqRenderViewBase *renderModule = qobject_cast<pqRenderViewBase *>(
        this->Display->getView());
    this->Form->ShowColorLegend->setEnabled(renderModule != 0);
    this->setLegend(this->ColorMap->getScalarBar(renderModule));

    // update opacity mapping gui elements
    this->handleEnableOpacityMappingChanged();
    }
}

void pqColorScaleEditor::pushColors()
{
  if(!this->ColorMap || this->Form->InSetColors)
    {
    return;
    }

  QList<QVariant> rgbPoints;
  this->Form->InSetColors = true;

  foreach(vtkColorTransferControlPointsItem* plot,
    this->ColorMapViewer->plots<vtkColorTransferControlPointsItem>())
    {
    vtkColorTransferFunction* tf=plot->GetColorTransferFunction();
    if ( ! tf ) continue;
    int total = tf->GetSize();
    double nodeValue[6];
    for(int i = 0; i < total; i++)
      {
      tf->GetNodeValue( i, nodeValue );
      rgbPoints
        << /* x */nodeValue[0]
        << /* R */nodeValue[1]
        << /* G */nodeValue[2]
        << /* B */nodeValue[3];
      }
    // If there is only one control point in the transfer function originally,
    // we need to add another control point.
    if(total == 1)
      {
      rgbPoints << nodeValue[0] << nodeValue[1] << nodeValue[2] << nodeValue[3];
      }
    }

  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
    lookupTable->GetProperty("RGBPoints"), rgbPoints);
  this->Form->InSetColors = false;
  lookupTable->UpdateVTKObjects();
  this->renderViewOptionally();
}

void pqColorScaleEditor::pushOpacity()
{
  if(!this->OpacityFunction || this->Form->InSetColors)
    {
    return;
    }

  QList<QVariant> opacityPoints;
  this->Form->InSetColors = true;

  double scalar[4];//[x, value, midpoint, sharpness]
  foreach(vtkCompositeControlPointsItem* plot,
    this->OpacityFunctionViewer->plots<vtkCompositeControlPointsItem>())
    {
    vtkPiecewiseFunction* pwf=plot->GetOpacityFunction();
    int total = pwf->GetSize();
    for(int i = 0; i < total; i++)
      {
      pwf->GetNodeValue(i, scalar);
      opacityPoints << scalar[0] << scalar[1] << scalar[2] << scalar[3];
      }
    }

  vtkSMProxy *points = this->OpacityFunction->getProxy();
  vtkSMDoubleVectorProperty* smProp = vtkSMDoubleVectorProperty::SafeDownCast(
    points->GetProperty("Points"));
  pqSMAdaptor::setMultipleElementProperty(smProp, opacityPoints);
  points->UpdateVTKObjects();

  this->Form->InSetColors = false;

  this->renderViewOptionally();
}

void pqColorScaleEditor::pushAnnotations()
{
  if ( ! this->ColorMap || this->Form->InSetAnnotation )
    {
    return;
    }

  vtkColorTransferFunction* tf = this->currentColorFunction();
  if ( tf )
    {
    tf->ResetAnnotations();
    }

  vtkSMProxy* lookupTable = this->ColorMap->getProxy();
  QList<QVariant> categories;
  QTreeWidgetItem* last = this->Form->AnnotationTree->topLevelItem(0);
  QTreeWidgetItem* valItem;
  // Find top-most item in list
  while (last && (valItem = this->Form->AnnotationTree->itemAbove(last)))
    {
    last = valItem;
    }
  valItem = last;
  // Traverse list from top down
  while (valItem)
    {
    QString val( valItem->data( PQ_ANN_VALUE_COL, Qt::DisplayRole ).toString() );
    QString txt( valItem->data( PQ_ANN_ENTRY_COL, Qt::DisplayRole ).toString() );
    categories << val << txt;
    if ( tf )
      {
      tf->SetAnnotation( val.toStdString(), txt.toStdString() );
      }
    valItem = this->Form->AnnotationTree->itemBelow(valItem);
    }
  this->Form->InSetAnnotation = true;
  pqSMAdaptor::setMultipleElementProperty(
    lookupTable->GetProperty( "Annotations" ), categories );

  lookupTable->UpdateVTKObjects();
  this->renderViewOptionally();
  this->Form->InSetAnnotation = false;
}

void pqColorScaleEditor::handleEnableOpacityMappingChanged()
{
  if(this->UseEnableOpacityCheckBox)
    {
    bool enabled = pqSMAdaptor::getElementProperty(
      this->ColorMap->getProxy()->GetProperty("EnableOpacityMapping")).toBool();

    this->Form->EnableOpacityFunction->setCheckState(
      enabled ? Qt::Checked : Qt::Unchecked);

    this->Form->frameOpacity->setVisible(enabled);
    }
}

void pqColorScaleEditor::handleOpacityPointsChanged()
{
  // If the point change was not generated by setColors, update the
  // points in the editor.
  if(!this->Form->InSetColors)
    {
    // Save the current point index to use after the change.
    vtkControlPointsItem* currentItem=this->OpacityFunctionViewer->
      currentControlPointsItem();
    int index = currentItem ?
      currentItem->GetCurrentPoint() : -1;

    // Load the new points.
    this->Form->IgnoreEditor = true;
    this->loadOpacityPoints();

    // Set the current point on the editor.
    if(index != -1 && this->OpacityFunctionViewer->currentControlPointsItem())
      {
      this->OpacityFunctionViewer->currentControlPointsItem()->SetCurrentPoint(index);
      }

    // Update the displayed values.
    this->Form->IgnoreEditor = false;
    this->enableOpacityPointControls();
    this->updateCurrentOpacityPoint();
    }
}

void pqColorScaleEditor::handleColorPointsChanged()
{
  // If the point change was not generated by setColors, update the
  // points in the editor.
  if(!this->Form->InSetColors)
    {
    // Save the index of the point currently being edited
    vtkControlPointsItem* currentItem=this->ColorMapViewer->
      currentControlPointsItem();
    int index = currentItem ?
      currentItem->GetCurrentPoint() : -1;

    // Load the new points.
    this->Form->IgnoreEditor = true;
    this->loadColorPoints();

    // Restore the current point being edited to its value before loadColorPoints() was called.
    if(index != -1 && this->ColorMapViewer->currentControlPointsItem())
      {
      this->ColorMapViewer->currentControlPointsItem()->SetCurrentPoint(index);
      }

    // Update the displayed values.
    this->Form->IgnoreEditor = false;
    this->updateCurrentColorPoint();
    }
}

/// Update the GUI radio buttons that indicate how to interpret the data when the SM proxy's IndexedLookup property changes
void pqColorScaleEditor::handleInterpretationChanged()
{
  // Only update the radio button states when the proxy change is *not* generated
  // by the user clicking on the radio buttons. :-)
  if ( ! this->Form->InSetInterpretation && this->ColorMap )
    {
    bool indexedLookup = this->ColorMap->getIndexedLookup();
    this->Form->updateInterpretation( indexedLookup );
    this->pushColors();
    }
}

/// A wrapper around loadAnnotations that saves (and restores) the selected annotations before (and after) loading.
void pqColorScaleEditor::handleAnnotationsChanged()
{
  if ( ! this->Form->InSetAnnotation )
    {
    this->loadAnnotations();
    }
}

void pqColorScaleEditor::setScalarFromText()
{
  vtkColorTransferFunction* colors = this->currentColorFunction();
  vtkControlPointsItem* currentItem=this->ColorMapViewer->
    currentControlPointsItem();
  int index = currentItem ?
    currentItem->GetCurrentPoint() : -1;

  if(index == -1 ||!colors)
    {
    return;
    }

  // Get the value from the line edit.
  bool ok = true;
  double value = this->Form->ScalarValue->text().toDouble(&ok);
  if(!ok)
    {
    // Reset to the previous value.
    this->updateCurrentColorPoint();
    return;
    }

  // Make sure the value is greater than the previous point and less
  // than the next point.
  double scalar[4];
  // check if the value is actually changed
  currentItem->GetControlPoint(index, scalar);
  if(scalar[0] == value)
    {
    return;
    }

  bool endpoint = index == 0;
  if(index > 0)
    {
    int i=index - 1;
    this->ColorMapViewer->currentControlPointsItem()->GetControlPoint(i, scalar);

    double prev = scalar[0];
    if(value <= prev)
      {
      // value not acceptable.
      this->updateCurrentColorPoint();
      return;
      }
    }

  endpoint = endpoint || index == colors->GetSize() - 1;
  if(index < colors->GetSize() - 1)
    {
    int i=index + 1;
    this->ColorMapViewer->currentControlPointsItem()->GetControlPoint(i, scalar);
    double next = scalar[0];
    if(value >= next)
      {
      // value not acceptable.
      this->updateCurrentColorPoint();
      return;
      }
    }

  double nodeVal[6];
  colors->GetNodeValue(index, nodeVal);
  nodeVal[0]=value;
  // Set the new value on the point in the editor.
  this->Form->IgnoreEditor = true;
  colors->SetNodeValue(index, nodeVal);
  this->Form->IgnoreEditor = false;

  // Update the colors on the proxy.
  this->pushColors();

  // Update the range if the modified point was an endpoint.
  if(endpoint)
    {
    QPair<double, double> range = this->ColorMap->getScalarRange();
    this->updateScalarRange(range.first, range.second);
    }

 // this->Viewer->Render();
}
void pqColorScaleEditor::setOpacityScalarFromText()
{
  vtkPiecewiseFunction* opacities = this->currentOpacityFunction();
  vtkControlPointsItem* currentItem=this->OpacityFunctionViewer->
    currentControlPointsItem();
  int index = currentItem ?
    currentItem->GetCurrentPoint() : -1;

  if(index == -1 ||!opacities)
    {
    return;
    }

  // Get the value from the line edit.
  bool ok = true;
  double value = this->Form->opacityScalar->text().toDouble(&ok);
  if(!ok)
    {
    // Reset to the previous value.
    this->updateCurrentOpacityPoint();
    return;
    }
  double scalar[4];
  // check if the value is actually changed
  currentItem->GetControlPoint(index, scalar);
  if(scalar[0] == value)
    {
    return;
    }

  scalar[0]=value;
  // Set the new value on the point in the editor.
  this->Form->IgnoreEditor = true;
  opacities->SetNodeValue(index, scalar);
  this->Form->IgnoreEditor = false;

  // Update the opacity on the proxy.
  this->pushOpacity();
}

void pqColorScaleEditor::setSingleOpacityFromText()
{
  if(!this->OpacityFunction)
    {
    return;
    }
  bool ok=true;
  double opacity = this->Form->Opacity->text().toDouble(&ok);
  if(!ok)
    {
    // Reset to the previous opacity.
    this->updateCurrentOpacityPoint();
    return;
    }

  QList<QVariant> opacityPoints;
  this->Form->InSetColors = true;
  vtkPiecewiseFunction* pwf=vtkPiecewiseFunction::SafeDownCast(
    this->OpacityFunction->getProxy()->GetClientSideObject());
  if(pwf)
    {
    // Make sure the opacity is valid (0.0 - 1.0).
    opacity = std::min(opacity, 1.0);
    opacity = std::max(opacity, 0.0);

    double scalar[4];//[x, value, midpoint, sharpness]
    int total = pwf->GetSize();
    for(int i = 0; i < total; i++)
      {
      pwf->GetNodeValue(i, scalar);
      scalar[1]=opacity;
      opacityPoints << scalar[0] << scalar[1] << scalar[2] << scalar[3];
      }
    vtkSMProxy *points = this->OpacityFunction->getProxy();
    vtkSMDoubleVectorProperty* smProp = vtkSMDoubleVectorProperty::SafeDownCast(
      points->GetProperty("Points"));
    pqSMAdaptor::setMultipleElementProperty(smProp, opacityPoints);
    points->UpdateVTKObjects();

    this->Form->InSetColors = false;

    this->renderViewOptionally();
    }
}

void pqColorScaleEditor::setOpacityFromText()
{
  if(this->OpacityFunction)
    {
    double range[2]={0,1};
    if(this->internalScalarRange(range) && range[0]==range[1])
      {
      this->setSingleOpacityFromText();
      return;
      }
    }

  vtkPiecewiseFunction *opacities = this->currentOpacityFunction();
  vtkControlPointsItem* currentItem=this->OpacityFunctionViewer->
    currentControlPointsItem();
  int index = currentItem ?
    currentItem->GetCurrentPoint() : -1;
  if(index == -1 || !this->OpacityFunction ||!opacities)
    {
    return;
    }

  // Get the opacity from the line edit.
  bool ok = true;
  double opacity = this->Form->Opacity->text().toDouble(&ok);
  if(!ok)
    {
    // Reset to the previous opacity.
    this->updateCurrentOpacityPoint();
    return;
    }

  // Make sure the opacity is valid (0.0 - 1.0).
  if(opacity < 0.0)
    {
    opacity = 0.0;
    }
  else if(opacity > 1.0)
    {
    opacity = 1.0;
    }

  // Set the new opacity on the point in the editor.
  this->Form->IgnoreEditor = true;

  double scalar[4];
  this->OpacityFunctionViewer->currentControlPointsItem()->GetControlPoint(
    index, scalar);
  scalar[1]=opacity;
  opacities->SetNodeValue(index, scalar);
  this->Form->IgnoreEditor = false;

  // Update the colors on the proxy.
  this->pushOpacity();
}
void pqColorScaleEditor::internalSetColorSpace(int index,
  vtkColorTransferFunction* colors)
{
  if(!colors)
    {
    return;
    }
  switch (index)
    {
    case 0:
      colors->SetColorSpaceToRGB();
      break;
    case 1:
      colors->SetColorSpaceToHSV();
      colors->HSVWrapOff();
      break;
    case 2:
      colors->SetColorSpaceToHSV();
      colors->HSVWrapOn();
      break;
    case 3:
      colors->SetColorSpaceToLab();
      break;
    case 4:
      colors->SetColorSpaceToDiverging();
      break;
    }
}
void pqColorScaleEditor::setColorSpace(int index)
{
  vtkColorTransferFunction* colors =this->currentColorFunction();
  if(this->ColorMap && colors)
    {
    this->internalSetColorSpace(index, colors);
    this->renderTransferFunctionViews();

    // Set the property on the lookup table.
    int wrap = index == 2 ? 1 : 0;
    if(index >= 2)
      {
      index--;
      }

    this->Form->InSetColors = true;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("ColorSpace"), index);
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("HSVWrap"), wrap);
    this->Form->InSetColors = false;
    lookupTable->UpdateVTKObjects();
    this->renderViewOptionally();
    }
}

void pqColorScaleEditor::setNanColor(const QColor &color)
{
  if (this->ColorMap)
    {
    this->Form->InSetColors = true;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    QList<QVariant> values;
    values << color.redF() << color.greenF() << color.blueF();
    pqSMAdaptor::setMultipleElementProperty(
                                  lookupTable->GetProperty("NanColor"), values);
    this->Form->InSetColors = false;
    lookupTable->UpdateVTKObjects();
    this->updateAnnotationColors();
    this->renderViewOptionally();
    this->renderTransferFunctionViews();
    }
  this->Form->NanColor2->blockSignals( true );
  this->Form->NanColor2->setChosenColor(color);
  this->Form->NanColor2->blockSignals( false );
}

void pqColorScaleEditor::setNanColor2(const QColor &color)
{
  if (this->ColorMap)
    {
    this->Form->InSetColors = true;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    QList<QVariant> values;
    values << color.redF() << color.greenF() << color.blueF();
    pqSMAdaptor::setMultipleElementProperty(
                                  lookupTable->GetProperty("NanColor"), values);
    this->Form->InSetColors = false;
    lookupTable->UpdateVTKObjects();
    this->updateAnnotationColors();
    this->renderViewOptionally();
    this->renderTransferFunctionViews();
    }
  this->Form->NanColor->blockSignals( true );
  this->Form->NanColor->setChosenColor(color);
  this->Form->NanColor->blockSignals( false );
}

void pqColorScaleEditor::setScalarColor(const QColor &color)
{
  if (!this->Form->InSetColors && this->ColorMap)
    {
    this->Form->InSetColors = true;
    vtkColorTransferFunction* clientTF=vtkColorTransferFunction::SafeDownCast(
      this->ColorMap->getProxy()->GetClientSideObject());
    if(!clientTF)
      {
      return;
      }
    this->setScalarButtonColor(color);

    int total = clientTF->GetSize();
    double nodeVal[6];
    QList<QVariant> rgbPoints;
    clientTF->GetNodeValue(total-1, nodeVal);
    nodeVal[1]=color.redF();
    nodeVal[2]=color.greenF();
    nodeVal[3]=color.blueF();
    clientTF->SetNodeValue(total-1, nodeVal);
    rgbPoints << nodeVal[0] << nodeVal[1] << nodeVal[2] <<nodeVal[3];
    for(int i = 0; i < total-1; i++)
      {
      clientTF->GetNodeValue(i, nodeVal);
      rgbPoints << nodeVal[0] << nodeVal[1] << nodeVal[2] <<nodeVal[3];
      }

    // If there is only one control point in the transfer function originally,
    // we need to add another control point.
    if(total == 1)
      {
      //// SHOULD NEVER GET HERE
      rgbPoints << nodeVal[0] << nodeVal[1] << nodeVal[2] <<nodeVal[3];
      }

    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setMultipleElementProperty(
      lookupTable->GetProperty("RGBPoints"), rgbPoints);
    lookupTable->UpdateVTKObjects();
    this->Form->InSetColors = false;
    this->renderViewOptionally();
    }
}

void pqColorScaleEditor::setScalarButtonColor(const QColor &color)
{
  this->Form->pushButtonScalarColor->setAutoFillBackground(true);
  QString stylesheet = "background-color: rgb(";
  stylesheet.append(QString::number(color.red())).append(", ").
    append(QString::number(color.green())).append(", ").
    append(QString::number(color.blue())).append("); border: none");
  this->Form->pushButtonScalarColor->setStyleSheet(stylesheet);
}

void pqColorScaleEditor::renderTransferFunctionViews()
{
  this->ColorMapViewer->renderView();
  if(this->OpacityFunction)
    {
    this->OpacityFunctionViewer->renderView();
    }
}

void pqColorScaleEditor::savePreset()
{
  // Get the color preset model from the manager.
  pqColorPresetModel *model = this->Form->Presets->getModel();

  // Save the current color scale settings as a preset.
  double scalar = 0.0;
  pqColorMapModel colorMap;
  colorMap.setColorSpaceFromInt( this->Form->ColorSpace->currentIndex() );
  colorMap.setIndexedLookup( this->Form->Interpretation->checkedId() == PQ_INTERPRET_CATEGORY );
  vtkColorTransferFunction* tf = this->currentColorFunction();
  vtkControlPointsItem* plot=this->ColorMapViewer->currentControlPointsItem();
  int total = tf->GetSize();
  double scalars[4];//[x, y, midpoint, sharpness]
  vtkPiecewiseFunction* pwf=this->currentOpacityFunction();
  double nodeValue[6]; // [x,r,g,b,smoothness]

  for(int i = 0; i < total; i++)
    {
    plot->GetControlPoint(i, scalars);
    scalar = scalars[0];
    tf->GetNodeValue(i, nodeValue);
    if(this->OpacityFunction && pwf)
      {
      double opacity = pwf->GetValue(scalar);
      colorMap.addPoint(pqChartValue(scalar),
          QColor::fromRgbF(nodeValue[1], nodeValue[2], nodeValue[3]), pqChartValue(opacity));
      }
    else
      {
      colorMap.addPoint(pqChartValue(scalar),
          QColor::fromRgbF(nodeValue[1], nodeValue[2], nodeValue[3]));
      }
    }
  colorMap.setNanColor( this->Form->NanColor->chosenColor() );

  QTreeWidget* atab = this->Form->AnnotationTree;
  total = atab->topLevelItemCount();
  for ( int i = 0; i < total; ++ i )
    {
    QTreeWidgetItem* valItem = atab->topLevelItem( i );
    if ( valItem )
      {
      QString val( valItem->data( PQ_ANN_VALUE_COL, Qt::DisplayRole ).toString() );
      QString txt( valItem->data( PQ_ANN_ENTRY_COL, Qt::DisplayRole ).toString() );
      colorMap.addAnnotation( val, txt );
      }
    }

  model->addColorMap(colorMap, "New Color Preset");

  // Select the newly added item (the last in the list).
  QItemSelectionModel *selection = this->Form->Presets->getSelectionModel();
  selection->setCurrentIndex(model->index(model->rowCount() - 1, 0),
      QItemSelectionModel::ClearAndSelect);

  // Set up the dialog and open it.
  this->Form->Presets->setUsingCloseButton(true);
  this->Form->Presets->exec();
}

void pqColorScaleEditor::loadPreset()
{
  // The apply is done automatically when selection change
  this->Form->Presets->show();
}

void pqColorScaleEditor::setLogScale(bool on)
{
  // Make sure both UI widget are in sync
  this->Form->UseLogScale->setChecked(on);
  this->Form->UseLogScaleSimple->setChecked(on);

  // Do the real job
  this->renderTransferFunctionViews();

  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  pqSMAdaptor::setElementProperty(
      lookupTable->GetProperty("UseLogScale"), on ? 1 : 0);

  // Set the log scale flag on the editor.
  this->currentColorFunction()->SetScale(
    on ? VTK_CTF_LOG10 : VTK_CTF_LINEAR);

  lookupTable->UpdateVTKObjects();
  this->renderViewOptionally();
}

void pqColorScaleEditor::setAutoRescale(bool on)
{
  // Make sure both UI widget are in sync
  this->Form->UseAutoRescale->setChecked(on);
  this->Form->UseAutoRescaleSimple->setChecked(on);
  this->Form->SimpleMin->setEnabled(!on);
  this->Form->SimpleMax->setEnabled(!on);

  // Do the real job
  this->enableRescaleControls(!on);
  this->ColorMap->setScalarRangeLock(!on);
  this->enablePointControls();
  if(on)
    {
    // Reset the range to the current.
    this->rescaleToDataRange();
    }
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::rescaleToNewRange()
{
  // Launch the rescale range dialog to get the new range.
  pqRescaleRange rescaleDialog(this);
  QPair<double, double> range = this->ColorMap->getScalarRange();
  rescaleDialog.setRange(range.first, range.second);
  if(rescaleDialog.exec() == QDialog::Accepted)
    {
    this->Form->InSetColors = true;
    this->unsetCurrentPoints();
    this->setScalarRange(rescaleDialog.getMinimum(),
        rescaleDialog.getMaximum());
    this->Form->InSetColors = false;
    range = this->ColorMap->getScalarRange();
    this->updateScalarRange(range.first, range.second);
    this->updateCurrentColorPoint();
    }
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::rescaleToDataRange()
{
  this->Form->InSetColors = true;
  this->unsetCurrentPoints();
  pqPipelineRepresentation *pipeline =
      qobject_cast<pqPipelineRepresentation *>(this->Display);
  if(pipeline)
    {
    pipeline->resetLookupTableScalarRange();
    pipeline->renderViewEventually();
    if(this->ColorMap)
      {
      QPair<double, double> range = this->ColorMap->getScalarRange();
      this->updateScalarRange(range.first, range.second);
      this->updateCurrentColorPoint();
      }
    }
  this->Form->InSetColors = false;
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::rescaleToDataRangeOverTime()
{
  this->Form->InSetColors = true;
  if (QMessageBox::warning(
      pqCoreUtilities::mainWidget(),
      "Potentially slow operation",
      "This can potentially take a long time to complete. \n"
      "Are you sure you want to continue?",
      QMessageBox::Yes |QMessageBox::No, QMessageBox::No) ==
    QMessageBox::Yes)
    {
    pqPipelineRepresentation *pipeline =
      qobject_cast<pqPipelineRepresentation *>(this->Display);
    if(pipeline)
      {
      this->unsetCurrentPoints();
      pipeline->resetLookupTableScalarRangeOverTime();
      pipeline->renderViewEventually();
      if(this->ColorMap)
        {
        QPair<double, double> range = this->ColorMap->getScalarRange();
        this->updateScalarRange(range.first, range.second);
        this->updateCurrentColorPoint();
        }
      }
    }
  this->Form->InSetColors = false;
  // TODO: Handle all the other representation types!
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::rescaleToSimpleRange()
{
  if(!this->Form->UseAutoRescaleSimple->isChecked() && !this->Form->AdvancedButton->isChecked())
    {
    bool okMin = true, okMax = true;
    double min = this->Form->SimpleMin->text().toDouble(&okMin);
    double max = this->Form->SimpleMax->text().toDouble(&okMax);
    if(okMin && okMax)
      {
      QPair<double, double> range;
      this->Form->InSetColors = true;
      this->unsetCurrentPoints();
      this->setScalarRange(min, max);
      this->Form->InSetColors = false;
      range = this->ColorMap->getScalarRange();
      this->updateScalarRange(range.first, range.second);
      this->updateCurrentColorPoint();
      }
    }
}

void pqColorScaleEditor::setScalarRange(double min, double max)
{
this->Form->InSetColors = true;
  // Update the opacity function if volume rendering.
  if(this->OpacityFunction)
    {
    this->OpacityFunction->setScalarRange(min, max);
    }

  // Update the color map and the rendered views.
  this->ColorMap->setScalarRange(min, max);
  this->Form->InSetColors = false;
  this->renderViewOptionally();
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::setUseDiscreteColors(bool on)
{
  // Update the color scale widget and gui controls.
  this->enableResolutionControls(on);

  if(this->ColorMap)
    {
    // Set the property on the lookup table.
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("Discretize"), (on ? 1 : 0));
    lookupTable->UpdateVTKObjects();
    this->renderViewOptionally();
    }
}

void pqColorScaleEditor::setSizeFromText()
{
  // Get the size from the text. Set the size for the slider and the
  // color scale.
  QString text = this->Form->TableSizeText->text();
  int tableSize = text.toInt();
  this->Form->TableSize->setValue(tableSize);
  this->setTableSize(tableSize);
}

void pqColorScaleEditor::setSizeFromSlider(int tableSize)
{
  QString sizeString;
  sizeString.setNum(tableSize);
  this->Form->TableSizeText->setText(sizeString);
  this->setTableSize(tableSize);
}

void pqColorScaleEditor::setTableSize(int tableSize)
{
  // TODO?
  //this->Viewer->Render();

  if(this->ColorMap)
    {
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("NumberOfTableValues"), QVariant(tableSize));
    lookupTable->UpdateVTKObjects();
    this->renderViewOptionally();
    }
}

void pqColorScaleEditor::resetAnnotations()
{
  this->Form->AnnotationTree->clear();
  this->pushAnnotations();
}

void pqColorScaleEditor::removeAnnotation()
{
  std::set<int> rows;
  QList<QTreeWidgetItem*> seln = this->Form->AnnotationTree->selectedItems();
  QList<QTreeWidgetItem*>::iterator it;
  bool removedSomething = false;
  for ( it = seln.begin(); it != seln.end(); ++ it )
    {
    int row = this->Form->AnnotationTree->indexOfTopLevelItem( *it );
    if ( row >= 0 )
      rows.insert( row );
    }
  for ( std::set<int>::iterator sit = rows.begin(); sit != rows.end(); ++ sit )
    {
    QTreeWidgetItem* item = this->Form->AnnotationTree->takeTopLevelItem( *sit );
    delete item;
    removedSomething = true;
    }
  if ( removedSomething )
    {
    this->pushAnnotations();
    }
}

void pqColorScaleEditor::addActiveValues()
{
  vtkAbstractArray* uniq = this->ActiveUniqueValues;
  if ( ! uniq )
    {
    return;
    }
  vtkIdType nv = uniq->GetNumberOfTuples();
  vtkIdType nr = this->Form->AnnotationTree->topLevelItemCount();
  int nc = uniq->GetNumberOfComponents();
  vtkIdType orig = nr;
  /*
  // Use something besides the decimal point as a separator for vector entries.
  char decsep = std::use_facet<std::numpunct<char> >(
    std::locale::classic()).decimal_point();
  char numsep = (decsep == '.' ? ',' : ';');
  */
  char numsep = ' '; // separator between array entries
  for ( vtkIdType i = nv - 1; i >= 0; -- i )
    {
    bool duplicate = false;
    QTreeWidgetItem* valItem;
    //QTreeWidgetItem* txtItem;
    std::ostringstream sval;
    vtkVariant comp;
    for ( int c = 0; c < nc; ++ c )
      {
      if ( c )
        {
        sval << numsep;
        }
      comp = uniq->GetVariantValue( i * nc + c );
      if ( comp.IsDouble() )
        {
        sval.precision(17);
        sval << comp.ToDouble();
        }
      else if ( comp.IsFloat() )
        {
        sval.precision(8);
        sval << comp.ToFloat();
        }
      else
        {
        sval << comp.ToString();
        }
      }
    for ( vtkIdType j = 0; j < nr; ++ j )
      {
      valItem = this->Form->AnnotationTree->topLevelItem( j );
      if ( valItem && valItem->data( PQ_ANN_VALUE_COL, Qt::DisplayRole ).toString().toStdString() == sval.str() )
        {
        duplicate = true;
        break;
        }
      }
    if ( ! duplicate )
      {
      valItem = new pqAnnotationTreeItem( this->Form->AnnotationTree );
      valItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
      vtkColorTransferFunction* tf = this->currentColorFunction();
      if ( tf && tf->GetSize() )
        {
        vtkColor3d color;
        if (tf->GetIndexedLookup())
          {
          double nodeValue[6];
          tf->GetNodeValue(nr % tf->GetSize(), nodeValue);
          for (int j = 0; j < 3; ++j)
            {
            color[j] = nodeValue[j+1];
            }
          }
        else
          {
          bool ok;
          double dval = comp.ToDouble(&ok);
          // String or vector values cannot be mapped to a double, so make them NaN
          if (!ok || uniq->GetNumberOfComponents() > 1)
            {
            // Retrieve the NaN color. Not all vtkScalarsToColors
            // subclasses have one, so we are stuck using GetColor.
            dval = vtkMath::Nan();
            }
          tf->GetColor(dval, color.GetData());
          }
        valItem->setData(PQ_ANN_VALUE_COL, Qt::DecorationRole, QColor::fromRgbF(color[0], color[1], color[2]));
        }
      valItem->setData( PQ_ANN_VALUE_COL, Qt::DisplayRole, QString( sval.str().c_str() ) );
      valItem->setData( PQ_ANN_ENTRY_COL, Qt::DisplayRole, QString( sval.str().c_str() ) );
      valItem->setFlags( valItem->flags() & ~(Qt::ItemIsDropEnabled) );
      this->Form->AnnotationTree->insertTopLevelItem( nr, valItem );
      ++ nr;
      }
    }
  // If we added any items, re-render.
  if ( nr != orig )
    {
    this->pushAnnotations();
    }
}

void pqColorScaleEditor::addAnnotationEntry()
{
  int nxtRow = this->Form->AnnotationTree->topLevelItemCount();
  QTreeWidgetItem* valItem = new pqAnnotationTreeItem( this->Form->AnnotationTree );
  vtkColorTransferFunction* tf = this->currentColorFunction();
  if ( tf && tf->GetSize() )
    {
    vtkColor3d color;
    if (tf->GetIndexedLookup())
      {
      double nodeValue[6];
      tf->GetNodeValue(nxtRow % tf->GetSize(), nodeValue);
      for (int j = 0; j < 3; ++j)
        {
        color[j] = nodeValue[j+1];
        }
      }
    else
      {
      double dval = vtkMath::Nan();
      tf->GetColor(dval, color.GetData());
      }
    valItem->setData(PQ_ANN_VALUE_COL, Qt::DecorationRole, QColor::fromRgbF(color[0], color[1], color[2]));
    }
  valItem->setData( PQ_ANN_VALUE_COL, Qt::DisplayRole, QString( "" ) );
  valItem->setData( PQ_ANN_ENTRY_COL, Qt::DisplayRole, QString( "Note" ) );
  valItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
  this->Form->AnnotationTree->insertTopLevelItem( nxtRow, valItem );
  this->Form->AnnotationTree->editItem( valItem, PQ_ANN_VALUE_COL );
  this->pushAnnotations();
}

void pqColorScaleEditor::annotationsChanged()
{
  // Reset annotations on local and proxy color transfer functions.
  this->pushAnnotations();
  this->updateAnnotationColors();
}

void pqColorScaleEditor::resetAnnotationSort()
{
  // Turn off sorting of annotations so drag-and-drop reordering will work.
  this->Form->AnnotationTree->sortByColumn( -1, Qt::DescendingOrder );
}

// When the order of annotations has changed, or the colors have changed,
// call this method to reassign colors to each annotation row.
void pqColorScaleEditor::updateAnnotationColors()
{
  QTreeWidget* atab = this->Form->AnnotationTree;
  vtkColorTransferFunction* tf = this->currentColorFunction();
  if ( ! tf || ! atab )
    {
    return;
    }
  atab->blockSignals( true );
  int nc = tf->GetSize();
  QTreeWidgetItem* last = this->Form->AnnotationTree->topLevelItem(0);
  QTreeWidgetItem* valItem;
  // Find top-most item in list
  while (last && (valItem = this->Form->AnnotationTree->itemAbove(last)))
    {
    last = valItem;
    }
  valItem = last;
  // Traverse list from top down
  for (int i = 0; valItem; ++i)
    {
    if ( nc )
      {
      vtkColor3d color;
      if (tf->GetIndexedLookup())
        {
        double nodeValue[6];
        tf->GetNodeValue(i % nc, nodeValue);
        for (int j = 0; j < 3; ++j)
          {
          color[j] = nodeValue[j+1];
          }
        }
      else
        {
        bool ok;
        double dval = valItem->data(PQ_ANN_VALUE_COL, Qt::DisplayRole).toDouble(&ok);
        // String or vector values cannot be mapped to a double, so make them NaN
        if (!ok)
          {
          // Retrieve the NaN color. Not all vtkScalarsToColors
          // subclasses have one, so we are stuck using GetColor.
          dval = vtkMath::Nan();
          }
        tf->GetColor(dval, color.GetData());
        }
      valItem->setData(PQ_ANN_VALUE_COL, Qt::DecorationRole, QColor::fromRgbF(color[0], color[1], color[2]));
      }
    else
      {
      valItem->setData( PQ_ANN_VALUE_COL, Qt::DecorationRole, this->Form->NanColor->chosenColor() );
      }
    valItem = this->Form->AnnotationTree->itemBelow(valItem);
    }
  this->annotationSelectionChanged(); // the selection stays the same, but its color changes...
  atab->blockSignals( false );
}

/// When the annotatation selected in the list changes, update the color chooser swatch to show it.
void pqColorScaleEditor::annotationSelectionChanged()
{
  QTreeWidget* atab = this->Form->AnnotationTree;
  if (!atab)
    {
    return;
    }
  QList<QTreeWidgetItem*> seln = atab->selectedItems();
  if (seln.size() < 1)
    {
    return;
    }
  QTreeWidgetItem* selectedItem = seln[0];
  QColor newColor = qVariantValue<QColor>(selectedItem->data(PQ_ANN_VALUE_COL, Qt::DecorationRole));
  this->Form->AnnotationSwatch->blockSignals(true);
  this->Form->AnnotationSwatch->setChosenColor(newColor);
  this->Form->AnnotationSwatch->blockSignals(false);
}

/**\brief When the annotation swatch color is changed by the user, update the appropriate color transfer control point.
 *
 * In interval/ratio mode, a new control point is added if one did not already exist
 * at the annotated value. If the annotated value cannot be converted to a double or
 * is NaN, then no action is taken. The control point's color is then modified.
 *
 * In categorical mode, the behavior is more complex in order to provide a way to change the number
 * of colors in the palette.
 * When the currently selected annotation index is larger than the number of control points in the color transfer
 * function, we add to the list of control points.
 * The addition goes as follows: if there are originally N control points and M>N annotations, with annotation P's color
 * being updated (where N>=P>M) then we add k multiples of N control points until kN>P.
 * The colors at the new control points duplicate the original N control points.
 */
void pqColorScaleEditor::editAnnotationColor(const QColor& newColor)
{
  QTreeWidget* atab = this->Form->AnnotationTree;
  vtkColorTransferFunction* tf = this->currentColorFunction();
  if (!tf || !atab)
    {
    return;
    }
  QList<QTreeWidgetItem*> seln = atab->selectedItems();
  if (seln.size() < 1)
    {
    return;
    }
  this->Form->AnnotationSwatch->blockSignals(true);
  QTreeWidgetItem* selectedItem = seln[seln.size() - 1];
  int nc = tf->GetSize();
  if (tf->GetIndexedLookup())
    {
    QTreeWidgetItem* last = this->Form->AnnotationTree->topLevelItem(0);
    QTreeWidgetItem* valItem;
    // Find top-most item in list
    while (last && (valItem = this->Form->AnnotationTree->itemAbove(last)))
      {
      last = valItem;
      }
    valItem = last;
    // Traverse list from top down searching for our item and counting colors along the way.
    for (int i = 0; valItem; ++i)
      {
      if ( valItem == selectedItem )
        {
        if ( nc )
          { // At least one transfer function control point.
          double nodeValue[6];
          if ( i >= nc )
            { // Need to add control points.
            double xRange[2];
            tf->GetRange(xRange);
            // Keep adding control points past point i until we reach a multiple of nc.
            int nc_mod = (((i + 1) / nc) + ((i + 1) % nc ? 1 : 0)) * nc;
            for (int j = nc; j < nc_mod; ++j)
              {
              tf->GetNodeValue(j % nc, nodeValue);
              tf->AddRGBPoint(xRange[1] + j + 1, nodeValue[1], nodeValue[2], nodeValue[3], nodeValue[4], nodeValue[5]);
              }
            // Now overwrite the i-th entry with the new color
            tf->GetNodeValue(i, nodeValue);
            nodeValue[1] = newColor.redF();
            nodeValue[2] = newColor.greenF();
            nodeValue[3] = newColor.blueF();
            tf->SetNodeValue(i, nodeValue);
            }
          else
            { // No need to add control points.
            tf->GetNodeValue( i % nc, nodeValue );
            nodeValue[1] = newColor.redF();
            nodeValue[2] = newColor.greenF();
            nodeValue[3] = newColor.blueF();
            tf->SetNodeValue( i, nodeValue );
            }
          }
        else
          {
          // No transfer function control points.
          // Insert NanColor entries up to our selection and then add the new color as the last entry.
          QColor nan(this->Form->NanColor->chosenColor());
          for (int j = 0; j < i - 1; ++j)
            {
            tf->AddRGBPoint(j / (i - 1.), nan.redF(), nan.greenF(), nan.blueF());
            }
          tf->AddRGBPoint(1., newColor.redF(), newColor.greenF(), newColor.blueF());
          }
        this->pushColors();
        this->updateAnnotationColors();
        break;
        }
      valItem = this->Form->AnnotationTree->itemBelow(valItem);
      }
    this->Form->AnnotationSwatch->blockSignals(false);
    }
  else
    {
    double xRange[2];
    tf->GetRange(xRange);
    double nodeValue[6];
    double xSel = selectedItem->data( PQ_ANN_VALUE_COL, Qt::DisplayRole ).toDouble();
    for (int i = 0; i < nc; ++i)
      {
      tf->GetNodeValue(i, nodeValue);
      if (nodeValue[0] == xSel)
        {
        nodeValue[1] = newColor.redF();
        nodeValue[2] = newColor.greenF();
        nodeValue[3] = newColor.blueF();
        tf->SetNodeValue(i, nodeValue);
        this->pushColors();
        this->updateAnnotationColors();
        return;
        }
      }
    // We made it here by not finding a matching control point. Add one:
    tf->AddRGBPoint(xSel, newColor.redF(), newColor.greenF(), newColor.blueF());
    this->pushColors();
    this->updateAnnotationColors();
    }
}

void pqColorScaleEditor::checkForLegend()
{
  if(!this->Form->MakingLegend && this->ColorMap)
    {
    pqRenderViewBase *view = qobject_cast<pqRenderViewBase *>(
        this->Display->getView());
    this->setLegend(this->ColorMap->getScalarBar(view));
    }
}

void pqColorScaleEditor::setLegendVisibility(bool visible)
{
  if(visible && !this->Legend)
    {
    if(this->ColorMap)
      {
      // Create a scalar bar in the current view. Use the display to
      // set up the title.
      this->Form->MakingLegend = true;
      pqLookupTableManager* lutManager =
        pqApplicationCore::instance()->getLookupTableManager();
      pqScalarBarRepresentation* legend = lutManager->setScalarBarVisibility(
        this->Display, visible);

      this->setLegend(legend);
      this->Form->MakingLegend = false;
      }
    else
      {
      qDebug() << "Error: No color map to add a color legend to.";
      }
    }

  if(this->Legend)
    {
    this->Legend->setVisible(visible);
    this->Legend->renderViewEventually();
    }

  this->Form->ShowColorLegend->blockSignals(true);
  this->Form->ShowColorLegend->setChecked(this->Legend && visible);
  this->Form->ShowColorLegend->blockSignals(false);
  this->enableLegendControls(this->Legend && visible);
}

void pqColorScaleEditor::updateLegendVisibility(bool visible)
{
  if(this->Legend)
    {
    this->Form->ShowColorLegend->blockSignals(true);
    this->Form->ShowColorLegend->setChecked(visible);
    this->Form->ShowColorLegend->blockSignals(false);
    }
}

void pqColorScaleEditor::setLegendName(const QString &text)
{
  this->setLegendTitle(text, this->Form->TitleComponent->text());
}

void pqColorScaleEditor::setLegendComponent(const QString &text)
{
  this->setLegendTitle(this->Form->TitleName->text(), text);
}

void pqColorScaleEditor::setLegendTitle(const QString &name,
    const QString &component)
{
  if(this->Legend)
    {
    this->Legend->setTitle(name, component);
    this->Legend->renderViewEventually();
    }
}

void pqColorScaleEditor::updateLegendTitle()
{
  if(this->Legend)
    {
    QPair<QString, QString> title = this->Legend->getTitle();
    this->Form->TitleName->blockSignals(true);
    this->Form->TitleName->setText(title.first);
    this->Form->TitleName->blockSignals(false);

    this->Form->TitleComponent->blockSignals(true);
    this->Form->TitleComponent->setText(title.second);
    this->Form->TitleComponent->blockSignals(false);
    }
}

void pqColorScaleEditor::cleanupDisplay()
{
  this->setRepresentation(0);
}

void pqColorScaleEditor::cleanupLegend()
{
  this->setLegend(0);
}

void pqColorScaleEditor::loadBuiltinColorPresets()
{
  this->Form->Presets->loadBuiltinColorPresets();
}

/// Update the GUI annotations table with values from the SM proxy.
void pqColorScaleEditor::loadAnnotations()
{
  QTreeWidget* anno = this->Form->AnnotationTree;
  if ( ! anno )
    {
    return;
    }

  // Only update from the proxy when we aren't responsible for the changes in the proxy.
  if ( this->Form->InSetAnnotation )
    {
    return;
    }

  // Empty previous entries.
  anno->clear();

  QList<QVariant> list;
  vtkSMProxy* lookupTable = this->ColorMap->getProxy();
  vtkSMStringVectorProperty* smProp = vtkSMStringVectorProperty::SafeDownCast(
    lookupTable->GetProperty( "Annotations" ) );
  int numPerCmd = smProp->GetNumberOfElementsPerCommand();
  if( numPerCmd != 2 )
    {
    return;
    }
  list = pqSMAdaptor::getMultipleElementProperty( smProp );
  this->Form->InSetAnnotation = true;
  for ( int i = 0; i < list.size() - 1; i += 2 )
    {
    QTreeWidgetItem* valItem = new pqAnnotationTreeItem( this->Form->AnnotationTree );
    valItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    valItem->setData( PQ_ANN_VALUE_COL, Qt::DisplayRole, list[i] );
    valItem->setData( PQ_ANN_ENTRY_COL, Qt::DisplayRole, list[i + 1] );
    valItem->setFlags( valItem->flags() & ~(Qt::ItemIsDropEnabled) );
    vtkColorTransferFunction* tf = this->currentColorFunction();
    if ( tf && tf->GetSize() )
      {
      vtkColor3d color;
      if (tf->GetIndexedLookup())
        {
        double nodeValue[6];
        tf->GetNodeValue((i / 2) % tf->GetSize(), nodeValue);
        for (int j = 0; j < 3; ++j)
          {
          color[j] = nodeValue[j+1];
          }
        }
      else
        {
        bool ok;
        double dval = list[i].toDouble(&ok);
        if (!ok)
          {
          // Retrieve the NaN color. Not all vtkScalarsToColors
          // subclasses have one, so we are stuck using GetColor.
          dval = vtkMath::Nan();
          }
        tf->GetColor(dval, color.GetData());
        }
      valItem->setData(PQ_ANN_VALUE_COL, Qt::DecorationRole, QColor::fromRgbF(color[0], color[1], color[2]));
      }
    }
  this->Form->InSetAnnotation = false;
}

void pqColorScaleEditor::loadColorPoints()
{
  vtkColorTransferFunction *colors = this->currentColorFunction();
  if(!colors)
    {
    return;
    }
  // Clean up the previous data.
  colors->RemoveAllPoints();

  if(this->ColorMap)
    {
    // Update the displayed min and max.
    QPair<double, double> range = this->ColorMap->getScalarRange();
    this->updateScalarRange(range.first, range.second);

    // Add the new data to the editor.
    QList<QVariant> list;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    vtkSMDoubleVectorProperty* smProp = vtkSMDoubleVectorProperty::SafeDownCast(
      lookupTable->GetProperty("RGBPoints"));
    int numPerCmd = smProp->GetNumberOfElementsPerCommand();
    if(numPerCmd != 4)
      {
      return;
      }
    list = pqSMAdaptor::getMultipleElementProperty(smProp);
    for(int i = 0; (i + 3) < list.size(); i += 4)
      {
      colors->AddRGBPoint(list[i].toDouble(), list[i + 1].toDouble(),
          list[i + 2].toDouble(), list[i + 3].toDouble());
      }
    }
  else
    {
    this->Form->MinimumLabel->setText("");
    this->Form->MaximumLabel->setText("");
    }
  this->updateAnnotationColors();
}
void pqColorScaleEditor::loadOpacityPoints()
{
  vtkPiecewiseFunction *opacities = this->currentOpacityFunction();
  if(!opacities || !this->OpacityFunction)
    {
    return;
    }
  opacities->RemoveAllPoints();
  // Add the new data to the editor.
  QList<QVariant> list;
  vtkSMDoubleVectorProperty* smProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->OpacityFunction->getProxy()->GetProperty("Points"));
  int numPerCmd = smProp->GetNumberOfElementsPerCommand();
  if(numPerCmd != 4)
    {
    return;
    }
  list = pqSMAdaptor::getMultipleElementProperty(smProp);
  for(int j = 0; (j + 3) < list.size(); j += 4)
    {
    opacities->AddPoint(list[j].toDouble(), list[j+1].toDouble(),
      list[j+2].toDouble(), list[j+3].toDouble());
    }
}

void pqColorScaleEditor::initColorScale()
{
  // Clear any pending changes and clear the current point index.

  // Ignore changes during editor setup.
  this->Form->IgnoreEditor = true;

  this->ColorMapViewer->blockSignals(true);
  this->OpacityFunctionViewer->blockSignals(true);
  // See if the display supports opacity editing.
  if(this->Display)
    {
    this->OpacityFunction = this->Display->getScalarOpacityFunction();

    if(this->OpacityFunction)
      {
      // the representation has a built-in opacity function which
      // cannot be disabled, so don't show the check box
      this->Form->EnableOpacityFunction->setVisible(false);
      this->UseEnableOpacityCheckBox = false;
      }
    else
      {
      pqScalarsToColors *colorMap = this->Display->getLookupTable();
      pqServerManagerModel* smmodel =
          pqApplicationCore::instance()->getServerManagerModel();
      pqSMProxy opacityFunctionProxy =
        pqSMAdaptor::getProxyProperty(
          colorMap->getProxy()->GetProperty("ScalarOpacityFunction"));
      if(opacityFunctionProxy.GetPointer())
        {
        this->OpacityFunction =
          smmodel->findItem<pqScalarOpacityFunction*>(opacityFunctionProxy);
        this->Form->EnableOpacityFunction->setVisible(true);
        this->UseEnableOpacityCheckBox = true;
        }
      }
    }

  bool usingOpacity = this->OpacityFunction != 0;
  if(this->OpacityFunction)
    {
    this->Form->frameOpacity->setVisible(1);
    this->ColorMapViewer->setSizePolicy(
      QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->Form->verticalSpacer->changeSize(20, 10,
      QSizePolicy::Expanding, QSizePolicy::Ignored);

    this->updateOpacityFunctionVisibility();

    this->Form->Listener->Connect(
        this->OpacityFunction->getProxy()->GetProperty("Points"),
        vtkCommand::ModifiedEvent, this, SLOT(handleOpacityPointsChanged()));
    if(this->Display->getProxy()->GetProperty("ScalarOpacityUnitDistance"))
      {
      this->Form->ReprLinks.addPropertyLink(
        this->Form->ScalarOpacityUnitDistance, "text", SIGNAL(editingFinished()),
        this->Display->getProxy(),
        this->Display->getProxy()->GetProperty("ScalarOpacityUnitDistance"));
      }
    }
  else
    {
    //this->ColorMapViewer->setSizePolicy(
    //  QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->Form->frameOpacity->setVisible(0);
    this->Form->verticalSpacer->changeSize(20, 10,
      QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
  this->setOpacityControlsVisibility(usingOpacity);

  if(this->ColorMap)
    {
    this->updateColorFunctionVisibility();
    QPair<double, double> range = this->ColorMap->getScalarRange();
    if(this->OpacityFunction)
      {
      this->OpacityFunction->setScalarRange(range.first, range.second);
      }
    this->updateScalarRange(range.first, range.second);

    // Set up the rescale controls.
    this->Form->UseAutoRescale->blockSignals(true);
    this->Form->UseAutoRescale->setChecked(
        !this->ColorMap->getScalarRangeLock());
    this->Form->UseAutoRescale->blockSignals(false);
    this->enableRescaleControls(!this->Form->UseAutoRescale->isChecked());

    // Handle simple UI
    this->Form->UseAutoRescaleSimple->blockSignals(true);
    this->Form->UseAutoRescaleSimple->setChecked(
        !this->ColorMap->getScalarRangeLock());
    this->Form->UseAutoRescaleSimple->blockSignals(false);
    this->Form->SimpleMin->setEnabled(!this->Form->UseAutoRescale->isChecked());
    this->Form->SimpleMax->setEnabled(!this->Form->UseAutoRescale->isChecked());

    // Set up the color table size elements.
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    int tableSize = pqSMAdaptor::getElementProperty(
      lookupTable->GetProperty("NumberOfTableValues")).toInt();
    this->Form->TableSize->blockSignals(true);
    this->Form->TableSize->setValue(tableSize);
    this->Form->TableSize->blockSignals(false);
    this->Form->TableSizeText->setText(QString::number(tableSize));

    int discretize = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("Discretize")).toInt();
    this->Form->UseDiscreteColors->blockSignals(true);
    this->Form->UseDiscreteColors->setChecked(discretize != 0);
    this->Form->UseDiscreteColors->blockSignals(false);
    this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());

    // Set up the color space combo box.
    int space = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("ColorSpace")).toInt();
    this->Form->ColorSpace->blockSignals(true);

    // Set the ColorSpace index, accounting for the fact that "HSVNoWrap" is
    // a fake that is inserted at index 2.
    if(pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("HSVWrap")).toInt())
      {
      this->Form->ColorSpace->setCurrentIndex(2);
      }
    else if (space >= 2)
      {
      this->Form->ColorSpace->setCurrentIndex(space+1);
      }
    else
      {
      this->Form->ColorSpace->setCurrentIndex(space);
      }

    this->Form->ColorSpace->blockSignals(false);

    this->internalSetColorSpace(this->Form->ColorSpace->currentIndex(),
      this->currentColorFunction());
    // Set up the NaN color.
    this->Form->NanColor->blockSignals(true);
    this->Form->NanColor2->blockSignals(true);
    QList<QVariant> nanColorValues = pqSMAdaptor::getMultipleElementProperty(
                                          lookupTable->GetProperty("NanColor"));
    QColor nanColor;
    nanColor.setRgbF(nanColorValues[0].toDouble(),
                     nanColorValues[1].toDouble(),
                     nanColorValues[2].toDouble());
    this->Form->NanColor->setChosenColor(nanColor);
    this->Form->NanColor2->setChosenColor(nanColor);
    this->Form->NanColor->blockSignals(false);
    this->Form->NanColor2->blockSignals(false);

    // Set up the log scale checkbox. If the log scale is not valid
    // because of the range, loadColorPoints will clear the flag.
    this->Form->UseLogScale->blockSignals(true);
    this->Form->UseLogScale->setChecked(pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("UseLogScale")).toInt() != 0);
    this->Form->UseLogScale->blockSignals(false);

    // Set the log scale flag on the editor.
    vtkColorTransferControlPointsItem* currentItem=
      vtkColorTransferControlPointsItem::SafeDownCast(
      this->ColorMapViewer->currentControlPointsItem());
    if(currentItem && currentItem->GetColorTransferFunction())
      {
      currentItem->GetColorTransferFunction()->SetScale(
        this->Form->UseLogScale->isChecked() ? VTK_CTF_LOG10 : VTK_CTF_LINEAR);
      }
    }

  // Update the displayed current point index.
  this->Form->IgnoreEditor = false;
  if(this->Display)
    {
    this->onColorPlotAdded(
      this->ColorMapViewer->currentControlPointsItem());
    this->ColorMapViewer->resetView();
    if(this->OpacityFunction)
      {
      this->onOpacityPlotAdded(
        this->OpacityFunctionViewer->currentControlPointsItem());
      this->OpacityFunctionViewer->resetView();
      }
    this->updatePointValues();
    }
  this->ColorMapViewer->blockSignals(false);
  this->OpacityFunctionViewer->blockSignals(false);

}

void pqColorScaleEditor::enablePointControls()
{
  this->enableColorPointControls();
  this->enableOpacityPointControls();
}

void pqColorScaleEditor::updatePointValues()
{
  this->Form->InSetColors = true;
  this->loadColorPoints();
  this->loadOpacityPoints();
  this->updateCurrentColorPoint();
  this->updateCurrentOpacityPoint();
  this->Form->InSetColors = false;
}

void pqColorScaleEditor::enableRescaleControls(bool enable)
{
  this->Form->RescaleButton->setEnabled(enable);
}

void pqColorScaleEditor::enableResolutionControls(bool enable)
{
  this->Form->TableSizeLabel->setEnabled(enable);
  this->Form->TableSize->setEnabled(enable);
  this->Form->TableSizeText->setEnabled(enable);
}

void pqColorScaleEditor::updateScalarRange(double min, double max)
{
  // Update the spin box ranges and set the values.
  this->Form->MinimumLabel->setText(QString::number(min, 'g', 6));
  this->Form->MaximumLabel->setText(QString::number(max, 'g', 6));
  this->Form->SimpleMin->setText(QString::number(min, 'g', 6));
  this->Form->SimpleMax->setText(QString::number(max, 'g', 6));

  double chartBounds[8];
  // Update the editor scalar range.
  vtkColorTransferFunction* colors = this->currentColorFunction();
  if(colors)
    {
    colors->SetAllowDuplicateScalars(1);
    this->ColorMapViewer->chartBounds(chartBounds);
    chartBounds[2] = min;
    chartBounds[3] = max;
    this->ColorMapViewer->setChartUserBounds(chartBounds);
    this->ColorMapViewer->resetView();
    if(this->currentOpacityFunction() &&
      this->OpacityFunctionViewer->isVisible())
      {
      this->OpacityFunctionViewer->chartBounds(chartBounds);
      chartBounds[2] = min;
      chartBounds[3] = max;
      this->OpacityFunctionViewer->setChartUserBounds(chartBounds);
      this->OpacityFunctionViewer->resetView();
      }
    }
  vtkPiecewiseFunction* pwf = this->currentOpacityFunction();
  if(pwf)
    {
    pwf->SetAllowDuplicateScalars(1);
    }
  if(this->ColorMap)
    {
    this->updateColorFunctionVisibility();
    }
  if(this->OpacityFunction)
    {
    this->updateOpacityFunctionVisibility();
    }
}

void pqColorScaleEditor::setLegend(pqScalarBarRepresentation *legend)
{
  if(this->Legend == legend)
    {
    return;
    }

  if(this->Legend)
    {
    // Clean up the current connections.
    this->disconnect(this->Legend, 0, this, 0);
    this->Form->Links.removeAllPropertyLinks();
    delete this->Form->TitleColorLink;
    this->Form->TitleColorLink = 0;
    delete this->Form->LabelColorLink;
    this->Form->LabelColorLink = 0;
    }

  this->Legend = legend;
  if(this->Legend)
    {
    this->connect(this->Legend, SIGNAL(destroyed(QObject *)),
        this, SLOT(cleanupLegend()));
    this->connect(this->Legend, SIGNAL(visibilityChanged(bool)),
        this, SLOT(updateLegendVisibility(bool)));

    // Connect the legend controls.
    vtkSMProxy *proxy = this->Legend->getProxy();
    this->Form->Links.addPropertyLink(this->Form->TitleColorAdaptor,
        "color", SIGNAL(colorChanged(const QVariant&)),
        proxy, proxy->GetProperty("TitleColor"));
    this->Form->Links.addPropertyLink(this->Form->TitleFontAdaptor,
        "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, proxy->GetProperty("TitleFontFamily"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleBold, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TitleBold"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleItalic, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TitleItalic"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleShadow, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TitleShadow"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleFontSize, "value", SIGNAL(editingFinished()),
        proxy, proxy->GetProperty("TitleFontSize"), 1);
    this->Form->Links.addPropertyLink(
        this->Form->TitleOpacity, "value", SIGNAL(valueChanged(double)),
        proxy, proxy->GetProperty("TitleOpacity"));

    this->Form->Links.addPropertyLink(this->Form->LabelColorAdaptor,
        "color", SIGNAL(colorChanged(const QVariant&)),
        proxy, proxy->GetProperty("LabelColor"));
    this->Form->Links.addPropertyLink(this->Form->LabelFontAdaptor,
        "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, proxy->GetProperty("LabelFontFamily"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelBold, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("LabelBold"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelItalic, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("LabelItalic"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelShadow, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("LabelShadow"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelFontSize, "value", SIGNAL(editingFinished()),
        proxy, proxy->GetProperty("LabelFontSize"), 1);
    this->Form->Links.addPropertyLink(
        this->Form->LabelOpacity, "value", SIGNAL(valueChanged(double)),
        proxy, proxy->GetProperty("LabelOpacity"));
    this->Form->Links.addPropertyLink(
        this->Form->AutomaticLabelFormat, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("AutomaticLabelFormat"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelFormat, "text", SIGNAL(editingFinished()),
        proxy, proxy->GetProperty("LabelFormat"));
    this->connect(this->Form->AutomaticLabelFormat, SIGNAL(toggled(bool)),
                  this, SLOT(updateLabelFormatControls()));
    this->updateLabelFormatControls();

    this->Form->Links.addPropertyLink(this->Form->NumberOfLabels,
        "value", SIGNAL(valueChanged(int)),
        proxy, proxy->GetProperty("NumberOfLabels"));
    this->Form->Links.addPropertyLink(this->Form->AspectRatio,
                                      "value", SIGNAL(valueChanged(double)),
                                      proxy, proxy->GetProperty("AspectRatio"));
    this->Form->Links.addPropertyLink(this->Form->DrawAnnotations,
        "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("DrawAnnotations"));
    this->Form->Links.addPropertyLink(this->Form->DrawNanAnnotation,
        "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("DrawNanAnnotation"));
    this->Form->Links.addPropertyLink(this->Form->NanAnnotation,
        "text", SIGNAL(textChanged(const QString&)),
        proxy, proxy->GetProperty("NanAnnotation"));
    this->Form->Links.addPropertyLink(this->Form->TextPosition,
        "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TextPosition"));

    // this manages the linking between the global properties and the color
    // properties.
    this->Form->TitleColorLink = new pqStandardColorLinkAdaptor(
      this->Form->TitleColorButton, proxy, "TitleColor");
    this->Form->LabelColorLink = new pqStandardColorLinkAdaptor(
      this->Form->LabelColorButton, proxy, "LabelColor");
    // Update the legend title gui.
    this->updateLegendTitle();
    }

  bool showing = this->Legend && this->Legend->isVisible();
  this->Form->ShowColorLegend->blockSignals(true);
  this->Form->ShowColorLegend->setChecked(showing);
  this->Form->ShowColorLegend->blockSignals(false);
  this->enableLegendControls(showing);
}

void pqColorScaleEditor::enableLegendControls(bool enable)
{
  this->Form->TitleFrame->setEnabled(enable);
  this->Form->LabelFrame->setEnabled(enable);
  this->Form->NumberOfLabels->setEnabled(enable);
  this->Form->CountLabel->setEnabled(enable);
  this->Form->AspectRatio->setEnabled(enable);
  this->Form->AspectRatioLabel->setEnabled(enable);
}

void pqColorScaleEditor::updateLabelFormatControls()
{
  bool autoFormat = this->Form->AutomaticLabelFormat->isChecked();
  this->Form->LabelFormatLabel->setEnabled(!autoFormat);
  this->Form->LabelFormat->setEnabled(!autoFormat);
}

void pqColorScaleEditor::makeDefault()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  if (lut_mgr)
    {
    lut_mgr->saveLUTAsDefault(this->ColorMap);
    if(this->OpacityFunction)
      {
      lut_mgr->saveOpacityFunctionAsDefault(this->OpacityFunction);
      }
    // BUG #13010: If scalar bar is visible, save the scalar bar's position as
    // default as well.
    if (this->Form->ShowColorLegend->isChecked() &&
      this->Legend)
      {
      lut_mgr->saveScalarBarAsDefault(this->Legend);
      }
    }
}
//-----------------------------------------------------------------------------
void pqColorScaleEditor::initTransferFunctionView()
{
  this->Form->ColorFunctionConnect->Disconnect();
  this->Form->OpacityFunctionConnect->Disconnect();
  this->ColorMapViewer->clearPlots();
  this->OpacityFunctionViewer->clearPlots();
  QObject::connect(this->ColorMapViewer, SIGNAL(plotAdded(vtkPlot*)),
    this, SLOT(onColorPlotAdded(vtkPlot*)));

  QObject::connect(this->OpacityFunctionViewer, SIGNAL(plotAdded(vtkPlot*)),
    this, SLOT(onOpacityPlotAdded(vtkPlot*)));
}
// ----------------------------------------------------------------------------
void pqColorScaleEditor::onColorPlotAdded(vtkPlot* plot)
{
  if (vtkControlPointsItem::SafeDownCast(plot))
    {
    this->Form->ColorFunctionConnect->Connect(plot,
      vtkControlPointsItem::CurrentPointChangedEvent,
      this, SLOT(updateColors()));
    QObject::connect(this->ColorMapViewer,
      SIGNAL(currentPointEdited()), this, SLOT(updateColors()));

    vtkColorTransferControlPointsItem* currentItem=
      vtkColorTransferControlPointsItem::SafeDownCast(plot);
    if(currentItem && currentItem->GetColorTransferFunction())
      {
      this->Form->ColorFunctionConnect->Connect(
        currentItem->GetColorTransferFunction(), vtkCommand::EndInteractionEvent,
        this, SLOT(updateColors()));
      }
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::onOpacityPlotAdded(vtkPlot* plot)
{
  if (vtkControlPointsItem::SafeDownCast(plot))
    {
    this->Form->OpacityFunctionConnect->Connect(plot,
      vtkControlPointsItem::CurrentPointChangedEvent,
      this, SLOT(updateOpacity()));
    vtkCompositeControlPointsItem* currentItem=
      vtkCompositeControlPointsItem::SafeDownCast(plot);
    if(currentItem && currentItem->GetOpacityFunction())
      {
      this->Form->OpacityFunctionConnect->Connect(
        currentItem->GetOpacityFunction(), vtkCommand::EndInteractionEvent,
        this, SLOT(updateOpacity()));
      }
    }
}
// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateCurrentColorPoint()
{
  this->enableColorPointControls();
  double range[2]={0,1};
  bool singleScalar = this->internalScalarRange(range) && range[0]==range[1];
  vtkColorTransferFunction* tf = this->currentColorFunction();
  vtkControlPointsItem* currentItem=this->ColorMapViewer->
    currentControlPointsItem();
  if(!currentItem || currentItem->GetNumberOfPoints() ==0 ||
     currentItem->GetCurrentPoint()<0)
    {
    this->Form->ScalarValue->setText("");
    }
  else
    {
    // if there is a valid color point, we need to disable
    // current opacity point if there is
    vtkControlPointsItem* currentOpaItem=
      this->OpacityFunctionViewer->currentControlPointsItem();
    if(currentOpaItem && !singleScalar)
      {
      currentOpaItem->SetCurrentPoint(-1);
      this->enableOpacityPointControls();
      }
    double scalar[4];
    int i=currentItem->GetCurrentPoint();
    currentItem->GetControlPoint(i, scalar);
    double value = scalar[0];
    this->Form->ScalarValue->setText(QString::number(value, 'g', 6));
    }
  // If there is only one scalar value, get the color
  // and set it to the ScalarColor button,
  // and set the scalar value to the text box
  if(tf && singleScalar)
    {
    double nodeVal[6];
    tf->GetNodeValue(0, nodeVal);
    double rgb[3]={nodeVal[1], nodeVal[2], nodeVal[3]};
    this->setScalarButtonColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
    this->Form->ScalarColor->setChosenColor(
          QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
    this->Form->ScalarValue->setText(QString::number(range[0], 'g', 6));
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateCurrentOpacityPoint()
{
  this->enableOpacityPointControls();
  double range[2]={0,1};
  bool singleScalar = this->internalScalarRange(range) && range[0]==range[1];

  vtkControlPointsItem* currentItem=this->OpacityFunctionViewer->
    currentControlPointsItem();
  if(!currentItem || currentItem->GetNumberOfPoints() ==0 ||
    currentItem->GetCurrentPoint()<0)
    {
    this->Form->Opacity->setText("");
    this->Form->opacityScalar->setText("");
    }
  else
    {
    if(this->OpacityFunction)
      {
      double scalar[4];
      int i=currentItem->GetCurrentPoint();
      if(i<0 || i>currentItem->GetNumberOfPoints())
        {
        this->Form->Opacity->setText("");
        this->Form->opacityScalar->setText("");
        }
      else
        {
        currentItem->GetControlPoint(i, scalar);
        double opacity = scalar[1];
        this->Form->Opacity->setText(QString::number(opacity, 'g', 6));
        double value = scalar[0];
        this->Form->opacityScalar->setText(QString::number(value, 'g', 6));
        vtkControlPointsItem* currentColorItem=
          this->ColorMapViewer->currentControlPointsItem();
        if(currentColorItem && !singleScalar)
          {
          currentColorItem->SetCurrentPoint(-1);
          this->enableColorPointControls();
          }
        }
      }
    else
      {
      this->Form->Opacity->setText("");
      this->Form->opacityScalar->setText("");
      }
    }

  // If there is only one scalar value, get the opacity
  // and set it to the ScalarColor button
  if(this->OpacityFunction && singleScalar)
    {
    vtkPiecewiseFunction* pwf=this->currentOpacityFunction();
    if(pwf)
      {
      double opacity = pwf->GetValue(range[0]);
      this->Form->Opacity->setText(QString::number(opacity, 'g', 6));
      this->Form->opacityScalar->setText(QString::number(range[0], 'g', 6));
      }
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::enableColorPointControls()
{
  vtkControlPointsItem* currentItem=this->ColorMapViewer->
    currentControlPointsItem();
  double range[2]={0,1};
  bool enable=false;
  if(this->internalScalarRange(range) && range[0]==range[1])
    {
    //if(currentItem && currentItem->GetNumberOfPoints()>0)
    //  {
    //  currentItem->SetCurrentPoint(0);
    //  }
    }
  else
    {
    int index = currentItem ? currentItem->GetCurrentPoint() : -1;
    enable = index != -1;

    // The endpoint values are not editable if auto rescale is on.
    if(enable && this->Form->UseAutoRescale->isChecked())
      {
      enable = index > 0;
      vtkIdType numPts = currentItem->GetNumberOfPoints();
      enable = enable && (index < numPts - 1);
      }
    }

  this->Form->ScalarValue->setEnabled(enable);
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::enableOpacityPointControls()
{
  vtkControlPointsItem* currentItem=this->OpacityFunctionViewer->
    currentControlPointsItem();
  double range[2]={0,1};
  bool enable=false;
  if(this->internalScalarRange(range) && range[0]==range[1])
    {
    if(currentItem && currentItem->GetNumberOfPoints()>0)
      {
      //currentItem->SetCurrentPoint(0);
      enable = true;
      }
    this->Form->opacityScalar->setEnabled(false);
    this->Form->labelOpacityScalar->setEnabled(false);
    }
  else
    {
    int index = currentItem ? currentItem->GetCurrentPoint() : -1;
    enable = index != -1;
    enable = this->OpacityFunction != 0 && enable;
    vtkIdType numPts = currentItem->GetNumberOfPoints();
    bool scalarenable = enable && index > 0 && index < numPts-1;
    this->Form->opacityScalar->setEnabled(scalarenable);
    this->Form->labelOpacityScalar->setEnabled(scalarenable);
    }
  this->Form->OpacityLabel->setEnabled(enable);
  this->Form->Opacity->setEnabled(enable);
}

// ----------------------------------------------------------------------------
vtkColorTransferFunction* pqColorScaleEditor::currentColorFunction()
{
  vtkColorTransferControlPointsItem* currentItem=
  vtkColorTransferControlPointsItem::SafeDownCast(
    this->ColorMapViewer->currentControlPointsItem());
  if(!currentItem)
    {
    return NULL;
    }
  return currentItem->GetColorTransferFunction();
}

// ----------------------------------------------------------------------------
vtkPiecewiseFunction* pqColorScaleEditor::currentOpacityFunction()
{
  vtkCompositeControlPointsItem* currentItem=
    vtkCompositeControlPointsItem::SafeDownCast(
    this->OpacityFunctionViewer->currentControlPointsItem());
  if(!currentItem)
    {
    return NULL;
    }
  return currentItem->GetOpacityFunction();
}

// ----------------------------------------------------------------------------
bool pqColorScaleEditor::internalScalarRange(double* range)
{
  if(!this->ColorMap)
    {
    return false;
    }

  QPair<double, double> curRange = this->ColorMap->getScalarRange();
  range[0] = curRange.first;
  range[1] = curRange.second;

  return true;
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateColorFunctionVisibility()
{
  double range[2]={0.0, 1.0};
  if(this->internalScalarRange(range))
    {
    if(range[0]==range[1])
      {
      this->ColorMapViewer->setVisible(0);
      this->Form->frameColorTF->setFrameShape(QFrame::Box);
      this->Form->pushButtonScalarColor->setVisible(1);
      }
    else
      {
      this->ColorMapViewer->setVisible(1);
      this->Form->frameColorTF->setFrameShape(QFrame::StyledPanel);
      this->Form->pushButtonScalarColor->setVisible(0);
      }
    vtkColorTransferFunction* ctf = vtkColorTransferFunction::SafeDownCast(
      this->ColorMap->getProxy()->GetClientSideObject());
    this->ColorMapViewer->setColorTransferFunctionToPlots(ctf);
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateOpacityFunctionVisibility()
{
  double range[2]={0.0, 1.0};
  if(this->internalScalarRange(range))
    {
    bool show_function_widget = (range[0] != range[1]);
    if (this->UseEnableOpacityCheckBox &&
      this->Form->EnableOpacityFunction->checkState() != Qt::Checked)
      {
      show_function_widget = false;
      }

    if (show_function_widget == false)
      {
      this->Form->frameOpacity->setVisible(0);
      this->Form->verticalSpacer->changeSize(20, 10,
        QSizePolicy::Expanding, QSizePolicy::Expanding);
      }
    else
      {
      this->Form->frameOpacity->setVisible(1);
      this->Form->verticalSpacer->changeSize(20, 10,
        QSizePolicy::Expanding, QSizePolicy::Ignored);
      }
    vtkPiecewiseFunction* otf = vtkPiecewiseFunction::SafeDownCast(
      this->OpacityFunction->getProxy()->GetClientSideObject());
    this->OpacityFunctionViewer->setOpacityFunctionToPlots(otf);
    vtkColorTransferFunction* ctf = vtkColorTransferFunction::SafeDownCast(
      this->ColorMap->getProxy()->GetClientSideObject());
    this->OpacityFunctionViewer->setColorTransferFunctionToPlots(ctf);
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::renderViewOptionally()
{
  if(this->Display && this->Form->checkBoxImmediateRender->isChecked())
    {
    this->Display->renderViewEventually();
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::saveOptionalUserSettings()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("ColorEditorSettings");
  settings->remove("");
  settings->setValue("ImmediateRender", QVariant(
    this->Form->checkBoxImmediateRender->isChecked()));
  settings->setValue("AdvancedPanel", QVariant(
    this->Form->AdvancedButton->isChecked()));
  settings->endGroup();
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::restoreOptionalUserSettings()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("ColorEditorSettings");
  QStringList keys = settings->childKeys();
  for(QStringList::Iterator key = keys.begin(); key != keys.end(); ++key)
    {
    if(*key == "ImmediateRender")
      {
      bool checked = settings->value(*key).toBool();
      this->Form->checkBoxImmediateRender->setChecked(checked);
      }
    if(*key == "AdvancedPanel")
      {
      bool checked = settings->value(*key).toBool();
      this->Form->AdvancedButton->setChecked(checked);
      }
    }

  settings->endGroup();
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::setOpacityControlsVisibility(bool visible)
{
  this->Form->ScaleLabel->setVisible(visible);
  this->Form->ScalarOpacityUnitDistance->setVisible(visible);
  this->Form->opacityScalar->setVisible(visible);
  this->Form->labelOpacityScalar->setVisible(visible);
  this->Form->OpacityLabel->setVisible(visible);
  this->Form->Opacity->setVisible(visible);
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::setEnableOpacityMapping(int enable)
{
  if(this->UseEnableOpacityCheckBox)
    {
    bool enabled = enable == Qt::Checked;
    pqSMAdaptor::setElementProperty(
      this->ColorMap->getProxy()->GetProperty("EnableOpacityMapping"), enabled);
    this->Form->frameOpacity->setVisible(enabled);
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::setInterpretation( int buttonId )
{
  bool indexedLookup = ( buttonId == PQ_INTERPRET_CATEGORY );
  this->Form->InSetInterpretation = true;
  vtkColorTransferFunction* colors = this->currentColorFunction();
  colors->SetIndexedLookup( indexedLookup );
  this->ColorMap->setIndexedLookup( indexedLookup );
  this->Form->updateInterpretation( indexedLookup );
  this->pushColors();
  this->Form->InSetInterpretation = false;
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateDisplay()
{
  if(this->Form->InSetColors)
    {
    return;
    }
  if(this->Display)
    {
    this->Form->InSetColors = true;
    if(this->ColorMap)
      {
      vtkSMProxy *lookupTable = this->ColorMap->getProxy();
      lookupTable->UpdateVTKObjects();
      }
    if(this->OpacityFunction)
      {
      vtkSMProxy *points = this->OpacityFunction->getProxy();
      points->UpdateVTKObjects();
      }

    this->Form->InSetColors = false;
    this->Display->renderViewEventually();
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::unsetCurrentPoints()
{
  vtkControlPointsItem* currentItem=this->ColorMapViewer->
    currentControlPointsItem();
  if(currentItem)
    {
    currentItem->SetCurrentPoint(-1);
    }
  currentItem=this->OpacityFunctionViewer->
    currentControlPointsItem();
  if(currentItem)
    {
    currentItem->SetCurrentPoint(-1);
    }
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::setActiveUniqueValues( vtkAbstractArray* arr )
{
  if ( this->ActiveUniqueValues == arr )
    return;

  if ( this->ActiveUniqueValues )
    {
    this->ActiveUniqueValues->Delete();
    }

  this->ActiveUniqueValues = arr;

  if ( this->ActiveUniqueValues )
    {
    this->ActiveUniqueValues->Register( 0 );
    std::ostringstream buttonTitle;
    vtkIdType nv = this->ActiveUniqueValues->GetNumberOfTuples();
    buttonTitle << "Add (" << nv << ") Active Values";
    this->Form->AddActiveValues->setText( buttonTitle.str().c_str() );
    this->Form->AddActiveValues->setEnabled( nv ? true : false );
    }
  else
    {
    this->Form->AddActiveValues->setText( "Too Many/Few Values" );
    this->Form->AddActiveValues->setEnabled( false );
    }
}

// ----------------------------------------------------------------------------
bool pqColorScaleEditor::eventFilter( QObject* src, QEvent* evnt )
{
  QObject* annotationTree = this->Form->AnnotationTree->viewport();
  if ( src == annotationTree )
    {
    if ( evnt->type() == QEvent::User )
      { // eat this event... it's something we queued below and we must respond to it now.
      this->pushAnnotations();
      this->updateAnnotationColors();
      evnt->setAccepted( true );
      return true;
      }
    else if ( evnt->type() == QEvent::Drop )
      { // Turn off sorting so the drop can reorder the rows.
      this->Form->AnnotationTree->sortByColumn( -1, Qt::DescendingOrder );
      }
    }
  bool retval = QObject::eventFilter( src, evnt );
  if ( src == annotationTree && evnt->type() == QEvent::Drop )
    {
    // We can't just pushAnnotations() here since the drop isn't complete...
    // instead, we'll add an event to the end of the queue (which won't be processed
    // until the drop completes) and call pushAnnotations() when we receive it.
    QApplication::postEvent( annotationTree, new QEvent( QEvent::User ) );
    }
  return retval;
}

// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateColors()
{
  this->updateCurrentColorPoint();
  this->pushColors();
}
// ----------------------------------------------------------------------------
void pqColorScaleEditor::updateOpacity()
{
  this->updateCurrentOpacityPoint();
  this->pushOpacity();
}
// ----------------------------------------------------------------------------
void pqColorScaleEditor::enableAdvancedPanel(bool checked)
{
  // Collapse
  this->Form->ColorTabs->setVisible(false);
  this->Form->SimplePanel->setVisible(false);

  // Expand eventually
  this->Form->ColorTabs->setVisible(checked);
  this->Form->SimplePanel->setVisible(!checked);

  // Adjust size
  this->adjustSize();
}
// ----------------------------------------------------------------------------
void pqColorScaleEditor::applyPreset()
{
  if( this->Form == NULL || this->Form->Presets == NULL ||
      this->Form->Presets->getSelectionModel() == NULL)
    {
    return;
    }
  // Get the color map from the selection.
  QItemSelectionModel *selection = this->Form->Presets->getSelectionModel();
  QModelIndex index = selection->currentIndex();
  const pqColorMapModel *colorMap =
      this->Form->Presets->getModel()->getColorMap(index.row());
  if(colorMap && this->ColorMap)
    {
    this->Form->IgnoreEditor = true;
    int colorSpace = colorMap->getColorSpaceAsInt();
    bool indexedLookup = colorMap->getIndexedLookup();

    QColor color;
    pqChartValue value, opacity;
    pqColorMapModel temp(*colorMap);
    if(this->Form->UseAutoRescale->isChecked() ||
        colorMap->isRangeNormalized())
      {
      QPair<double, double> range = this->ColorMap->getScalarRange();
      temp.setValueRange(range.first, range.second);
      }

    vtkPiecewiseFunction *opacities = NULL;
    vtkColorTransferFunction* colors = this->currentColorFunction();
    this->ColorMapViewer->currentControlPointsItem()->SetCurrentPoint(-1);
    if(this->OpacityFunction)
      {
      opacities = this->currentOpacityFunction();
      this->OpacityFunctionViewer->currentControlPointsItem()->
        SetCurrentPoint(-1);
      opacities->RemoveAllPoints();
      }

    // Update the displayed range.
    temp.getValueRange(value, opacity);
    this->updateScalarRange(value.getDoubleValue(), opacity.getDoubleValue());
    bool singleScalar = (value.getDoubleValue()==opacity.getDoubleValue());
    int numPoints = colorMap->getNumberOfPoints();
    if(colors && numPoints > 0)
      {
      colors->RemoveAllPoints();
      if(singleScalar)
        {
        if(numPoints>1)
          {
          temp.getPointColor(numPoints-1, color);
          colors->AddRGBPoint(value.getDoubleValue(), color.redF(),
            color.greenF(), color.blueF());
          }
        temp.getPointColor(0, color);
        colors->AddRGBPoint(value.getDoubleValue(), color.redF(),
          color.greenF(), color.blueF());
        }
      else
        {
        for(int i = 0; i < numPoints; i++)
          {
          temp.getPointColor(i, color);
          temp.getPointValue(i, value);
          colors->AddRGBPoint(value.getDoubleValue(), color.redF(),
            color.greenF(), color.blueF());
          if(this->OpacityFunction)
            {
            temp.getPointOpacity(i, opacity);
            opacities->AddPoint(value.getDoubleValue(),
              opacity.getDoubleValue());
            }
          }
        }
      colors->SetIndexedLookup(indexedLookup);
      }

    // Update the color space.
    this->internalSetColorSpace(colorSpace, colors);

    // Update the color space chooser.
    this->Form->ColorSpace->blockSignals(true);
    this->Form->ColorSpace->setCurrentIndex(colorSpace);
    this->Form->ColorSpace->blockSignals(false);
    if(this->ColorMap)
      {
      // Set the property on the lookup table.
      int wrap = colorSpace == 2 ? 1 : 0;
      if(colorSpace >= 2)
        {
        colorSpace--;
        }

      this->Form->InSetColors = true;
      vtkSMProxy *lookupTable = this->ColorMap->getProxy();
      pqSMAdaptor::setElementProperty(
          lookupTable->GetProperty("ColorSpace"), colorSpace);
      pqSMAdaptor::setElementProperty(
          lookupTable->GetProperty("HSVWrap"), wrap);
      this->Form->InSetColors = false;
      }

    // Update the NaN color.
    QColor nanColor;
    colorMap->getNanColor(nanColor);
    this->Form->NanColor->blockSignals(true);
    this->Form->NanColor->setChosenColor(nanColor);
    this->Form->NanColor2->setChosenColor(nanColor);
    this->Form->NanColor->blockSignals(false);

    if (this->ColorMap)
      {
      // Set the property on the lookup table.
      this->Form->InSetColors = true;
      vtkSMProxy *lookupTable = this->ColorMap->getProxy();
      QList<QVariant> values;
      values << nanColor.redF() << nanColor.greenF() << nanColor.blueF();
      pqSMAdaptor::setMultipleElementProperty(
                                lookupTable->GetProperty("NanColor"), values);
      this->Form->InSetColors = false;
      }

    // IndexedLookup: Update the GUI via the proxy
    this->Form->updateInterpretation( indexedLookup );
    // IndexedLookup: Update the proxy
    this->setInterpretation( indexedLookup ? PQ_INTERPRET_CATEGORY : PQ_INTERPRET_INTERVAL );

    // Annotations: Update the proxy
    QList<QVariant> annotations = colorMap->getAnnotations();
    // Only change the annotations if the preset has some.
    // Otherwise, keep the current set of annotations.
    if (annotations.size() > 0)
      {
      this->Form->InSetAnnotation = true;
      this->ColorMap->setAnnotations(annotations);
      this->Form->InSetAnnotation = false;
      // Annotations: Update the GUI via the proxy
      this->handleAnnotationsChanged();
      }
    else
      {
      this->addActiveValues();
      }

    // Update the actual color map.
    this->Form->IgnoreEditor = false;

    if(singleScalar)
      {
      // the color to set on the color button
      this->Form->ScalarColor->blockSignals(true);
      this->Form->ScalarColor->setChosenColor(color);
      this->Form->ScalarColor->blockSignals(false);
      this->setScalarColor(color);
      }
    else
      {
      this->pushColors();
      }

    this->updatePointValues();
    }
  this->updateDisplay();
}
