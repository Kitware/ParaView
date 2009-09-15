/*=========================================================================

Program: ParaView
Module:    PrismSurfacePanel.cxx

=========================================================================*/

#include "PrismSurfacePanel.h"

// Qt includes
#include <QTreeWidget>
#include <QVariant>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QKeyEvent>

// VTK includes

// ParaView Server Manager includes
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMDoubleVectorProperty.h"
// ParaView includes
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqPropertyManager.h"
#include "ui_PrismSurfacePanelWidget.h"
#include "pqScalarSetModel.h"
#include "pqSampleScalarAddRangeDialog.h"
class PrismSurfacePanel::pqUI : public QObject, public Ui::PrismSurfacePanelWidget 
{
public:
    pqUI(PrismSurfacePanel* p) : QObject(p)
    {
        // Make a clone of the XDMFReader proxy.
        // We'll use the clone to help us with the interdependent properties.
        // In other words, modifying properties outside of accept()/reset() is wrong.
        // We have to modify properties to get the information we need
        // and we'll do that with the clone.
        vtkSMProxyManager* pm = vtkSMProxy::GetProxyManager();
        PanelHelper.TakeReference(pm->NewProxy("misc", "SESAMEReaderHelper"));
        PanelHelper->InitializeAndCopyFromProxy(p->proxy());
        this->PanelHelper->UpdatePropertyInformation();
    }
    // our helper
    vtkSmartPointer<vtkSMProxy> PanelHelper;
    pqScalarSetModel Model;
};

//----------------------------------------------------------------------------
PrismSurfacePanel::PrismSurfacePanel(pqProxy* object_proxy, QWidget* p) :
pqNamedObjectPanel(object_proxy, p)
{
    this->UI = new pqUI(this);
    this->UI->setupUi(this);

    QObject::connect(this->UI->TableIdWidget, SIGNAL(currentIndexChanged(QString)), 
        this, SLOT(setTableId(QString)));

    QObject::connect(this->UI->XLogScaling, SIGNAL(toggled (bool)),
        this, SLOT(useXLogScaling(bool)));
    QObject::connect(this->UI->YLogScaling, SIGNAL(toggled (bool)),
        this, SLOT(useYLogScaling(bool)));


    QObject::connect(this->UI->ThresholdXBetweenLower, SIGNAL(valueEdited(double)),
        this, SLOT(lowerXChanged(double)));
    QObject::connect(this->UI->ThresholdXBetweenUpper, SIGNAL(valueEdited(double)),
        this, SLOT(upperXChanged(double)));

    QObject::connect(this->UI->ThresholdYBetweenLower, SIGNAL(valueEdited(double)),
        this, SLOT(lowerYChanged(double)));
    QObject::connect(this->UI->ThresholdYBetweenUpper, SIGNAL(valueEdited(double)),
        this, SLOT(upperYChanged(double)));


    //watch for changes in the widget so that we can tell the proxy
    QObject::connect(this->UI->XAxisVarName, SIGNAL(currentIndexChanged(QString)), 
        this, SLOT(setXVariable(QString)));

    //watch for changes in the widget so that we can tell the proxy
    QObject::connect(this->UI->YAxisVarName, SIGNAL(currentIndexChanged(QString)), 
        this, SLOT(setYVariable(QString)));
    //watch for changes in the widget so that we can tell the proxy
    QObject::connect(this->UI->ZAxisVarName, SIGNAL(currentIndexChanged(QString)), 
        this, SLOT(setZVariable(QString)));
    QObject::connect(this->UI->ContourVarName, SIGNAL(currentIndexChanged(QString)), 
        this, SLOT(setContourVariable(QString)));



  this->UI->Model.setPreserveOrder(false);
  this->UI->Values->setModel(&this->UI->Model);
  this->UI->Values->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->UI->Values->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->UI->Delete->setEnabled(false);
  this->UI->Values->installEventFilter(this);
  

  connect(
    this->UI->Values->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    this,
    SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(
    this->UI->Delete,
    SIGNAL(clicked()),
    this,
    SLOT(onDelete()));
   connect(
    this->UI->DeleteAll,
    SIGNAL(clicked()),
    this,
    SLOT(onDeleteAll()));
    
  connect(
    this->UI->NewValue,
    SIGNAL(clicked()),
    this,
    SLOT(onNewValue()));
    
  connect(
    this->UI->NewRange,
    SIGNAL(clicked()),
    this,
    SLOT(onNewRange()));
   
    
  connect(
    this->UI->ScientificNotation,
    SIGNAL(toggled(bool)),
    this,
    SLOT(onScientificNotation(bool)));

    connect(
    &this->UI->Model,
    SIGNAL(layoutChanged()),
    this,
    SLOT(onSamplesChanged()));

  this->onSamplesChanged();




    this->linkServerManagerProperties();



}

//----------------------------------------------------------------------------
PrismSurfacePanel::~PrismSurfacePanel()
{
}

void PrismSurfacePanel::accept()
{

    QComboBox* tableWidget = this->UI->TableIdWidget;

    pqSMAdaptor::setElementProperty(
        this->proxy()->GetProperty("TableId"), tableWidget->currentText());

    QComboBox* xVariables = this->UI->XAxisVarName;
    QComboBox* yVariables = this->UI->YAxisVarName;
    QComboBox* zVariables = this->UI->ZAxisVarName;
    QComboBox* cVariables = this->UI->ContourVarName;



    pqSMAdaptor::setElementProperty(
        this->proxy()->GetProperty("XAxisVariableName"),  xVariables->currentText());
    pqSMAdaptor::setElementProperty(
        this->proxy()->GetProperty("YAxisVariableName"),  yVariables->currentText());
    pqSMAdaptor::setElementProperty(
        this->proxy()->GetProperty("ZAxisVariableName"),  zVariables->currentText());
    pqSMAdaptor::setElementProperty(
        this->proxy()->GetProperty("ContourVariableName"),  cVariables->currentText());

    vtkSMDoubleVectorProperty* xThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->proxy()->GetProperty("ThresholdXBetween"));

    if(xThresholdVP)
    {
        xThresholdVP->SetElement(0,this->UI->ThresholdXBetweenLower->value());
        xThresholdVP->SetElement(1,this->UI->ThresholdXBetweenUpper->value());
    }

    vtkSMDoubleVectorProperty* yThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->proxy()->GetProperty("ThresholdYBetween"));

    if(yThresholdVP)
    {
        yThresholdVP->SetElement(0,this->UI->ThresholdYBetweenLower->value());
        yThresholdVP->SetElement(1,this->UI->ThresholdYBetweenUpper->value());
    }


    vtkSMDoubleVectorProperty* contourValueVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->proxy()->GetProperty("ContourValues"));


    const QList<double> sample_list = this->UI->Model.values();
    contourValueVP->SetNumberOfElements(sample_list.size());
    for(int i = 0; i != sample_list.size(); ++i)
    {
        contourValueVP->SetElement(i, sample_list[i]);
    }
 
 



    this->proxy()->UpdateVTKObjects();
    this->proxy()->UpdatePropertyInformation();


    pqNamedObjectPanel::accept();

}

//----------------------------------------------------------------------------
void PrismSurfacePanel::reset()
{


    // clear possible changes in helper


    this->setupTableWidget();
    this->setupVariables();
    this->setupXThresholds();
    this->setupYThresholds();


    pqNamedObjectPanel::reset();
}

//----------------------------------------------------------------------------
void PrismSurfacePanel::linkServerManagerProperties()
{
    this->setupTableWidget();
    this->setupVariables();
    this->updateXThresholds();
    this->updateYThresholds();






    // parent class hooks up some of our widgets in the ui
    pqNamedObjectPanel::linkServerManagerProperties();
}

void PrismSurfacePanel::setupTableWidget()
{
    //empty the selection widget on the UI (and don't call the changed slot)
    QComboBox* tableWidget = this->UI->TableIdWidget;
    tableWidget->blockSignals(true);

    tableWidget->clear();
    //watch for changes in the widget so that we can tell the proxy

    vtkSMProperty* GetNamesProperty = this->proxy()->GetProperty("TableIds");
    QList<QVariant> names;
    names = pqSMAdaptor::getMultipleElementProperty(GetNamesProperty);

    //add each xdmf-domain name to the widget and to the paraview-Domain
    foreach(QVariant v, names)
    {
        tableWidget->addItem(v.toString());
    }

    // get the current value
    vtkSMProperty* SetTableIdProperty = this->proxy()->GetProperty("TableId");
    QVariant str = pqSMAdaptor::getEnumerationProperty(SetTableIdProperty);

    if(str.toString().isEmpty())
    {
        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("TableId"),
            tableWidget->currentText());
        this->UI->PanelHelper->UpdateVTKObjects();
        this->UI->PanelHelper->UpdatePropertyInformation();
    }
    else
    {
        // set the combo box to the current
        tableWidget->setCurrentIndex(tableWidget->findText(str.toString()));
    }
    tableWidget->blockSignals(false);

}

void PrismSurfacePanel::updateXThresholds()
{
    this->UI->ThresholdXBetweenLower->blockSignals(true);
    this->UI->ThresholdXBetweenUpper->blockSignals(true);


    vtkSMProperty* prop = this->UI->PanelHelper->GetProperty("XAxisRange");
    vtkSMDoubleVectorProperty* xRangeVP = vtkSMDoubleVectorProperty::SafeDownCast(prop);
    if(xRangeVP)
    {

        this->UI->ThresholdXBetweenLower->setMinimum(xRangeVP->GetElement(0));
        this->UI->ThresholdXBetweenLower->setMaximum(xRangeVP->GetElement(1));
        this->UI->ThresholdXBetweenUpper->setMinimum(xRangeVP->GetElement(0));
        this->UI->ThresholdXBetweenUpper->setMaximum(xRangeVP->GetElement(1));

        this->UI->ThresholdXBetweenLower->setValue(xRangeVP->GetElement(0));
        this->UI->ThresholdXBetweenUpper->setValue(xRangeVP->GetElement(1));
    }

    this->UI->ThresholdXBetweenLower->blockSignals(false);
    this->UI->ThresholdXBetweenUpper->blockSignals(false);
}
void PrismSurfacePanel::setupXThresholds()
{
    this->UI->ThresholdXBetweenLower->blockSignals(true);
    this->UI->ThresholdXBetweenUpper->blockSignals(true);


    vtkSMProperty* prop = this->UI->PanelHelper->GetProperty("XAxisRange");
    vtkSMDoubleVectorProperty* xRangeVP = vtkSMDoubleVectorProperty::SafeDownCast(prop);
    if(xRangeVP)
    {

        this->UI->ThresholdXBetweenLower->setMinimum(xRangeVP->GetElement(0));
        this->UI->ThresholdXBetweenLower->setMaximum(xRangeVP->GetElement(1));
        this->UI->ThresholdXBetweenUpper->setMinimum(xRangeVP->GetElement(0));
        this->UI->ThresholdXBetweenUpper->setMaximum(xRangeVP->GetElement(1));
    }

    vtkSMDoubleVectorProperty* xHelperThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdXBetween"));

    vtkSMDoubleVectorProperty* xThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->proxy()->GetProperty("ThresholdXBetween"));

    if(xThresholdVP && xHelperThresholdVP)
    {
        this->UI->ThresholdXBetweenLower->setValue(xThresholdVP->GetElement(0));
        this->UI->ThresholdXBetweenUpper->setValue(xThresholdVP->GetElement(1));

        xHelperThresholdVP->SetElement(0,xThresholdVP->GetElement(0));
        xHelperThresholdVP->SetElement(1,xThresholdVP->GetElement(1));
    }


    this->UI->ThresholdXBetweenLower->blockSignals(false);
    this->UI->ThresholdXBetweenUpper->blockSignals(false);
}

void PrismSurfacePanel::updateYThresholds()
{
    this->UI->ThresholdYBetweenLower->blockSignals(true);
    this->UI->ThresholdYBetweenUpper->blockSignals(true);

    vtkSMProperty* yProp = this->UI->PanelHelper->GetProperty("YAxisRange");
    vtkSMDoubleVectorProperty* yRangeVP = vtkSMDoubleVectorProperty::SafeDownCast(yProp);
    if(yRangeVP)
    {
        this->UI->ThresholdYBetweenLower->setMinimum(yRangeVP->GetElement(0));
        this->UI->ThresholdYBetweenLower->setMaximum(yRangeVP->GetElement(1));
        this->UI->ThresholdYBetweenUpper->setMinimum(yRangeVP->GetElement(0));
        this->UI->ThresholdYBetweenUpper->setMaximum(yRangeVP->GetElement(1));

        this->UI->ThresholdYBetweenLower->setValue(yRangeVP->GetElement(0));
        this->UI->ThresholdYBetweenUpper->setValue(yRangeVP->GetElement(1));
    }


    this->UI->ThresholdYBetweenLower->blockSignals(false);
    this->UI->ThresholdYBetweenUpper->blockSignals(false);
}
void PrismSurfacePanel::setupYThresholds()
{
    this->UI->ThresholdYBetweenLower->blockSignals(true);
    this->UI->ThresholdYBetweenUpper->blockSignals(true);

    vtkSMProperty* yProp = this->UI->PanelHelper->GetProperty("YAxisRange");

    vtkSMDoubleVectorProperty* yRangeVP = vtkSMDoubleVectorProperty::SafeDownCast(yProp);
    if(yRangeVP)
    {
        this->UI->ThresholdYBetweenLower->setMinimum(yRangeVP->GetElement(0));
        this->UI->ThresholdYBetweenLower->setMaximum(yRangeVP->GetElement(1));
        this->UI->ThresholdYBetweenUpper->setMinimum(yRangeVP->GetElement(0));
        this->UI->ThresholdYBetweenUpper->setMaximum(yRangeVP->GetElement(1));
    }

    vtkSMDoubleVectorProperty* yHelperThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdYBetween"));

    vtkSMDoubleVectorProperty* yThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->proxy()->GetProperty("ThresholdYBetween"));

    if(yThresholdVP  && yHelperThresholdVP)
    {
        this->UI->ThresholdYBetweenLower->setValue(yThresholdVP->GetElement(0));
        this->UI->ThresholdYBetweenUpper->setValue(yThresholdVP->GetElement(1));

        yHelperThresholdVP->SetElement(0,yThresholdVP->GetElement(0));
        yHelperThresholdVP->SetElement(1,yThresholdVP->GetElement(1));


    }

    this->UI->ThresholdYBetweenLower->blockSignals(false);
    this->UI->ThresholdYBetweenUpper->blockSignals(false);
}

void PrismSurfacePanel::updateVariables()
{
    QComboBox* xVariables = this->UI->XAxisVarName;
    QComboBox* yVariables = this->UI->YAxisVarName;
    QComboBox* zVariables = this->UI->ZAxisVarName;
    QComboBox* cVariables = this->UI->ContourVarName;

    xVariables->blockSignals(true);
    yVariables->blockSignals(true);
    zVariables->blockSignals(true);
    cVariables->blockSignals(true);


    xVariables->clear();
    yVariables->clear();
    zVariables->clear();
    cVariables->clear();


    vtkSMProperty* GetNamesProperty =this->UI->PanelHelper->GetProperty("AxisVarNameInfo");
    QList<QVariant> names;
    names = pqSMAdaptor::getMultipleElementProperty(GetNamesProperty);

    //add each xdmf-domain name to the widget and to the paraview-Domain
    foreach(QVariant v, names)
    {
        xVariables->addItem(v.toString());
        yVariables->addItem(v.toString());
        zVariables->addItem(v.toString());
        cVariables->addItem(v.toString());
    }


    // get the current value
    vtkSMProperty* xVariableProperty =this->UI->PanelHelper->GetProperty("XAxisVariableName");
    QVariant str = pqSMAdaptor::getEnumerationProperty(xVariableProperty);

    if(str.toString().isEmpty())
    {
        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("XAxisVariableName"),
            xVariables->currentText());



    }
    else
    {
        // set the combo box to the current
        int index=xVariables->findText(str.toString());

        if(index==-1)
        {
            xVariables->setCurrentIndex(0);
            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("XAxisVariableName"),
                xVariables->currentText());

        }
        else
        {
            xVariables->setCurrentIndex(index);
        }


    }



    // get the current value
    vtkSMProperty* yVariableProperty = this->UI->PanelHelper->GetProperty("YAxisVariableName");
    str = pqSMAdaptor::getEnumerationProperty(yVariableProperty);

    if(str.toString().isEmpty())
    {
        if(names.size()>=2)
        {
            yVariables->setCurrentIndex(1);
        }  
        else
        {
            yVariables->setCurrentIndex(0);
        }

        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("YAxisVariableName"),
            yVariables->currentText());
    }
    else
    {
        int index=yVariables->findText(str.toString());
        if(index==-1)
        {

            if(names.size()>=2)
            {
                yVariables->setCurrentIndex(1);
            } 
            else
            {
                yVariables->setCurrentIndex(0);
            }

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("YAxisVariableName"),
                yVariables->currentText());


        }
        else
        {
            yVariables->setCurrentIndex(index);
        }

    }




    vtkSMProperty* zVariableProperty = this->proxy()->GetProperty("ZAxisVariableName");
    str = pqSMAdaptor::getEnumerationProperty(zVariableProperty);

    if(str.toString().isEmpty())
    {

        if(names.size()>=3)
        {
            zVariables->setCurrentIndex(2);
        }

        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("ZAxisVariableName"),
            zVariables->currentText());
        this->UI->PanelHelper->UpdateVTKObjects();
        this->UI->PanelHelper->UpdatePropertyInformation();
    }
    else
    {
        int index=zVariables->findText(str.toString());

        if(index==-1)
        {

            if(names.size()>=3)
            {
                zVariables->setCurrentIndex(2);
            }  

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("ZAxisVariableName"),
                zVariables->currentText());


        }
        else
        {
            zVariables->setCurrentIndex(index);
        }
    }


    vtkSMProperty* cVariableProperty = this->proxy()->GetProperty("ContourVariableName");
    str = pqSMAdaptor::getEnumerationProperty(cVariableProperty);

    if(str.toString().isEmpty())
    {

        if(names.size()>=4)
        {
            cVariables->setCurrentIndex(3);
        }
        else
        {
            cVariables->setCurrentIndex(0);
        }

        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("ContourVariableName"),
            cVariables->currentText());
    }
    else
    {
        int index=zVariables->findText(str.toString());

        if(index==-1)
        {

            if(names.size()>=4)
            {
                cVariables->setCurrentIndex(3);
            }  
            else
            {
                cVariables->setCurrentIndex(0);

            }

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("ContourVariableName"),
                cVariables->currentText());


        }
        else
        {
            cVariables->setCurrentIndex(index);
        }
    }

    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();


    xVariables->blockSignals(false);
    yVariables->blockSignals(false);
    zVariables->blockSignals(false);
    cVariables->blockSignals(false);
}


//----------------------------------------------------------------------------
void PrismSurfacePanel::setupVariables()
{
    QComboBox* xVariables = this->UI->XAxisVarName;
    QComboBox* yVariables = this->UI->YAxisVarName;
    QComboBox* zVariables = this->UI->ZAxisVarName;
    QComboBox* cVariables = this->UI->ContourVarName;

    xVariables->blockSignals(true);
    yVariables->blockSignals(true);
    zVariables->blockSignals(true);
    cVariables->blockSignals(true);

    xVariables->clear();
    yVariables->clear();
    zVariables->clear();
    cVariables->clear();

    vtkSMProperty* GetNamesProperty =this->proxy()->GetProperty("AxisVarNameInfo");
    QList<QVariant> names;
    names = pqSMAdaptor::getMultipleElementProperty(GetNamesProperty);

    //add each xdmf-domain name to the widget and to the paraview-Domain
    foreach(QVariant v, names)
    {
        xVariables->addItem(v.toString());
        yVariables->addItem(v.toString());
        zVariables->addItem(v.toString());
        cVariables->addItem(v.toString());
    }


    // get the current value
    vtkSMProperty* xVariableProperty = this->proxy()->GetProperty("XAxisVariableName");
    QVariant str = pqSMAdaptor::getEnumerationProperty(xVariableProperty);

    QString temp=str.toString();
    if(str.toString().isEmpty())
    {
        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("XAxisVariableName"),
            xVariables->currentText());


    }
    else
    {
        // set the combo box to the current
        int index=xVariables->findText(str.toString());
        if(index==-1)
        {
            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("XAxisVariableName"),
                xVariables->currentText());
        }
        else
        {
            xVariables->setCurrentIndex(index);


            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("XAxisVariableName"),
                xVariables->currentText());


        }
    }




    // get the current value
    vtkSMProperty* yVariableProperty = this->proxy()->GetProperty("YAxisVariableName");
    str = pqSMAdaptor::getEnumerationProperty(yVariableProperty);

    if(str.toString().isEmpty())
    {

        if(names.size()>=2)
        {
            yVariables->setCurrentIndex(1);
        }  

        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("YAxisVariableName"),
            yVariables->currentText());


    }
    else
    {
        int index=yVariables->findText(str.toString());
        if(index==-1)
        {

            if(names.size()>=2)
            {
                yVariables->setCurrentIndex(1);
            }  

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("YAxisVariableName"),
                yVariables->currentText());


        }
        else
        {
            yVariables->setCurrentIndex(index);

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("YAxisVariableName"),
                yVariables->currentText());
        }



    }



    // get the current value
    vtkSMProperty* zVariableProperty = this->proxy()->GetProperty("ZAxisVariableName");
    str = pqSMAdaptor::getEnumerationProperty(zVariableProperty);

    if(str.toString().isEmpty())
    {

        if(names.size()>=3)
        {
            zVariables->setCurrentIndex(2);
        }

        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("ZAxisVariableName"),
            zVariables->currentText());
    }
    else
    {
        // set the combo box to the current
        int index=zVariables->findText(str.toString());

        if(index==-1)
        {

            if(names.size()>=3)
            {
                zVariables->setCurrentIndex(2);
            }  

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("ZAxisVariableName"),
                zVariables->currentText());


        }
        else
        {
            zVariables->setCurrentIndex(index);
            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("ZAxisVariableName"),
                zVariables->currentText());


        }    
    }

    // get the current value
    vtkSMProperty* cVariableProperty = this->proxy()->GetProperty("ContourVariableName");
    str = pqSMAdaptor::getEnumerationProperty(cVariableProperty);

    if(str.toString().isEmpty())
    {

        if(names.size()>=4)
        {
            cVariables->setCurrentIndex(3);
        }

        // initialize our helper to whatever item is current
        pqSMAdaptor::setElementProperty(
            this->UI->PanelHelper->GetProperty("ContourVariableName"),
            cVariables->currentText());
    }
    else
    {
        // set the combo box to the current
        int index=cVariables->findText(str.toString());

        if(index==-1)
        {

            if(names.size()>=4)
            {
                cVariables->setCurrentIndex(3);
            }  

            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("ContourVariableName"),
                cVariables->currentText());


        }
        else
        {
            cVariables->setCurrentIndex(index);
            pqSMAdaptor::setElementProperty(
                this->UI->PanelHelper->GetProperty("ContourVariableName"),
                cVariables->currentText());


        }    
    }


    // Set the list of values
    QList<double> values;
    vtkSMDoubleVectorProperty* contourValueVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->proxy()->GetProperty("ContourValues"));

    if(contourValueVP)
    {
        const int value_count = contourValueVP->GetNumberOfElements();
        for(int i = 0; i != value_count; ++i)
        {
            values.push_back(contourValueVP->GetElement(i));
        }
    }

    this->UI->Model.clear();
    for(int i = 0; i != values.size(); ++i)
    {
        this->UI->Model.insert(values[i]);
    }
      this->UI->PanelHelper->UpdateVTKObjects();
       this->UI->PanelHelper->UpdatePropertyInformation();

    this->onRangeChanged();
    //    this->UI->PanelHelper->UpdateVTKObjects();
    //   this->UI->PanelHelper->UpdatePropertyInformation();







    xVariables->blockSignals(false);
    yVariables->blockSignals(false);
    zVariables->blockSignals(false);
    cVariables->blockSignals(false);
}


void PrismSurfacePanel::setTableId(QString newId)
{

    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("TableId"), newId);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();

    this->updateVariables();
    this->updateXThresholds();
    this->updateYThresholds();

    this->setModified();


}

void PrismSurfacePanel::setXVariable(QString name)
{
    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("XAxisVariableName"), name);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();

    this->updateXThresholds();

    this->setModified();
}
void PrismSurfacePanel::setYVariable(QString name)
{
    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("YAxisVariableName"), name);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();

    this->updateYThresholds();

    this->setModified();

}
void PrismSurfacePanel::setZVariable(QString name)
{
    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("ZAxisVariableName"), name);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();
    this->setModified();
}


void PrismSurfacePanel::setContourVariable(QString name)
{
    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("ContourVariableName"), name);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();
    this->onRangeChanged();
    this->setModified();
}
void PrismSurfacePanel::lowerXChanged(double val)
{
    if(this->UI->ThresholdXBetweenUpper->value() < val)
    {
        this->UI->ThresholdXBetweenUpper->setValue(val);
    }

    vtkSMDoubleVectorProperty* xThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdXBetween"));

    if(xThresholdVP)
    {
        xThresholdVP->SetElement(0,this->UI->ThresholdXBetweenLower->value());
        xThresholdVP->SetElement(1,this->UI->ThresholdXBetweenUpper->value());
    }
    vtkSMDoubleVectorProperty* yThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdYBetween"));

    if(yThresholdVP)
    {
        yThresholdVP->SetElement(0,this->UI->ThresholdYBetweenLower->value());
        yThresholdVP->SetElement(1,this->UI->ThresholdYBetweenUpper->value());
    }

    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();
    this->setModified();

}

void PrismSurfacePanel::upperXChanged(double val)
{
    if(this->UI->ThresholdXBetweenLower->value() > val)
    {
        this->UI->ThresholdXBetweenLower->setValue(val);
    }
    vtkSMDoubleVectorProperty* xThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdXBetween"));

    if(xThresholdVP)
    {
        xThresholdVP->SetElement(0,this->UI->ThresholdXBetweenLower->value());
        xThresholdVP->SetElement(1,this->UI->ThresholdXBetweenUpper->value());
    }
    vtkSMDoubleVectorProperty* yThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdYBetween"));

    if(yThresholdVP)
    {
        yThresholdVP->SetElement(0,this->UI->ThresholdYBetweenLower->value());
        yThresholdVP->SetElement(1,this->UI->ThresholdYBetweenUpper->value());
    }
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();
    this->setModified();
}
void PrismSurfacePanel::lowerYChanged(double val)
{
    if(this->UI->ThresholdYBetweenUpper->value() < val)
    {
        this->UI->ThresholdYBetweenUpper->setValue(val);
    }

    vtkSMDoubleVectorProperty* xThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdXBetween"));

    if(xThresholdVP)
    {
        xThresholdVP->SetElement(0,this->UI->ThresholdXBetweenLower->value());
        xThresholdVP->SetElement(1,this->UI->ThresholdXBetweenUpper->value());
    }

    vtkSMDoubleVectorProperty* yThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdYBetween"));

    if(yThresholdVP)
    {
        yThresholdVP->SetElement(0,this->UI->ThresholdYBetweenLower->value());
        yThresholdVP->SetElement(1,this->UI->ThresholdYBetweenUpper->value());
    }
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();
    this->setModified();
}
void PrismSurfacePanel::upperYChanged(double val)
{
    if(this->UI->ThresholdYBetweenLower->value() > val)
    {
        this->UI->ThresholdYBetweenLower->setValue(val);
    }

    vtkSMDoubleVectorProperty* xThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdXBetween"));

    if(xThresholdVP)
    {
        xThresholdVP->SetElement(0,this->UI->ThresholdXBetweenLower->value());
        xThresholdVP->SetElement(1,this->UI->ThresholdXBetweenUpper->value());
    }

    vtkSMDoubleVectorProperty* yThresholdVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UI->PanelHelper->GetProperty("ThresholdYBetween"));

    if(yThresholdVP)
    {
        yThresholdVP->SetElement(0,this->UI->ThresholdYBetweenLower->value());
        yThresholdVP->SetElement(1,this->UI->ThresholdYBetweenUpper->value());
    }
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();
    this->setModified();
}

void PrismSurfacePanel::useXLogScaling( bool b)
{
    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("XLogScaling"), b);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();

    this->updateXThresholds();

    this->setModified();
}
void PrismSurfacePanel::useYLogScaling(bool b)
{
    //get access to the property that lets us pick the domain
    pqSMAdaptor::setElementProperty(
        this->UI->PanelHelper->GetProperty("YLogScaling"), b);
    this->UI->PanelHelper->UpdateVTKObjects();
    this->UI->PanelHelper->UpdatePropertyInformation();

    this->updateYThresholds();
    this->setModified();

}
void PrismSurfacePanel::onSamplesChanged()
{
   this->UI->DeleteAll->setEnabled(
    this->UI->Model.values().size());

this->setModified();

}

void PrismSurfacePanel::onRangeChanged()
{
  double range_min;
  double range_max;
  if(this->getRange(range_min, range_max))
    {
    this->UI->ScalarRange->setText(
      tr("Value Range: [%1, %2]").arg(range_min).arg(range_max));
    }
  else
    {
    this->UI->ScalarRange->setText(
      tr("Value Range: unlimited"));
    }
 this->onSamplesChanged();
}

bool PrismSurfacePanel::getRange(double& range_min, double& range_max)
{

    vtkSMProperty* prop = this->UI->PanelHelper->GetProperty("ContourVarRange");
    vtkSMDoubleVectorProperty* cRangeVP = vtkSMDoubleVectorProperty::SafeDownCast(prop);
    if(cRangeVP)
    {
        range_min=cRangeVP->GetElement(0);
        range_max=cRangeVP->GetElement(1);

        return true;
    }


    return false;
}


void PrismSurfacePanel::onSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  this->UI->Delete->setEnabled(
    this->UI->Values->selectionModel()->selectedIndexes().size());
}

void PrismSurfacePanel::onDelete()
{
  QList<int> rows;
  for(int i = 0; i != this->UI->Model.rowCount(); ++i)
    {
    if(this->UI->Values->selectionModel()->isRowSelected(i, QModelIndex()))
      rows.push_back(i);
    }

  for(int i = rows.size() - 1; i >= 0; --i)
    {
    this->UI->Model.erase(rows[i]);
    }

  this->UI->Values->selectionModel()->clear();
  
  this->onSamplesChanged();
 }
void PrismSurfacePanel::onDeleteAll()
{
  this->UI->Model.clear();
 
  this->UI->Values->selectionModel()->clear();
  
  this->onSamplesChanged();
}
void PrismSurfacePanel::onNewValue()
{
  double new_value = 0.0;
  QList<double> values = this->UI->Model.values();
  if(values.size())
    {
    double delta = 0.1;
    if(values.size() > 1)
      {
      delta = values[values.size() - 1] - values[values.size() - 2];
      }
    new_value = values[values.size() - 1] + delta;
    }
    
  QModelIndex idx=this->UI->Model.insert(new_value);
  
  this->UI->Values->setCurrentIndex(idx);
  this->UI->Values->edit(idx);
 this->onSamplesChanged();
}

void PrismSurfacePanel::onNewRange()
{
  double current_min = 0.0;
  double current_max = 1.0;
  this->getRange(current_min, current_max);
  
  pqSampleScalarAddRangeDialog dialog(current_min, current_max, 10, false);
  if(QDialog::Accepted != dialog.exec())
    {
    return;
    }
    
  const double from = dialog.from();
  const double to = dialog.to();
  const unsigned long steps = dialog.steps();
  const bool logarithmic = dialog.logarithmic();

  if(steps < 2)
    return;
    
  if(from == to)
    return;

  if(logarithmic)
    {
    const double sign = from < 0 ? -1.0 : 1.0;
    const double log_from = log10(fabs(from ? from : 1.0e-6 * (from - to)));
    const double log_to = log10(fabs(to ? to : 1.0e-6 * (to - from)));
    
    for(unsigned long i = 0; i != steps; ++i)
      {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);
      this->UI->Model.insert(sign * pow(10.0, (1.0 - mix) * log_from + (mix) * log_to));
      }
    }
  else
    {
    for(unsigned long i = 0; i != steps; ++i)
      {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);
      this->UI->Model.insert((1.0 - mix) * from + (mix) * to);
      }
    }
   this->onSamplesChanged();

}

void PrismSurfacePanel::onSelectAll()
{
  for(int i = 0; i != this->UI->Model.rowCount(); ++i)
    {
    this->UI->Values->selectionModel()->select(
      this->UI->Model.index(i, 0),
      QItemSelectionModel::Select);
    }
}

void PrismSurfacePanel::onScientificNotation(bool enabled)
{
  if(enabled)
    {
    this->UI->Model.setFormat('e');
    }
  else
    {
    this->UI->Model.setFormat('g');
    }
}

bool PrismSurfacePanel::eventFilter(QObject *object, QEvent *e)
{
  if(object == this->UI->Values && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    if(keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
      {
       this->onDelete();
      }
    }

  return QWidget::eventFilter(object, e);
}





