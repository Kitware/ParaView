/*=========================================================================

Program: ParaView
Module:    PrismScaleViewDialog.cxx

=========================================================================*/
#include "PrismScaleViewDialog.h"
#include "ui_PrismScaleViewWidget.h"

#include <QRadioButton>
#include <QButtonGroup>
#include <QSignalMapper>

class PrismScaleViewDialog::pqInternals : public Ui::PrismScaleViewWidget
{
public:
  QButtonGroup XAxesBG;
  QButtonGroup YAxesBG;
  QButtonGroup ZAxesBG;

  QSignalMapper SigMap;
  QSignalMapper

  int ScalingMode[3];
  double CustomBounds[6];
  pqInternals()
    {
    this->ScalingMode[0] = 0;
    this->ScalingMode[1] = 0;
    this->ScalingMode[2] = 0;
    this->CustomBounds[0] = 0;
    this->CustomBounds[1] = 0;
    this->CustomBounds[2] = 0;
    this->CustomBounds[3] = 0;
    this->CustomBounds[4] = 0;
    this->CustomBounds[5] = 0;
    }
};

//-----------------------------------------------------------------------------
PrismScaleViewDialog::PrismScaleViewDialog(
  QWidget* parentObject, Qt::WindowFlags _flags)
  : Superclass(parentObject, _flags),
  View(view)
{
  double worldBounds[6], thresholdBounds[6], double customBounds[6];
  pview->GetWorldBounds(worldBounds);
  pview->GetThresholdBounds(thresholdBounds);
  pview->GetCustomBounds(customBounds);
  
  int currentAxesMode[3];
  pview->GetCurrentAxesMode(currentAxesMode);
  
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  //Setup the button groups so we have a group for each axes
  this->Internals->XAxesBG->AddButton(this->Internals->UseXFullBounds);
  this->Internals->XAxesBG->AddButton(this->Internals->UseXThresholdBounds);
  this->Internals->XAxesBG->AddButton(this->Internals->UseXCustomScale);

  this->Internals->YAxesBG->AddButton(this->Internals->UseYFullBounds);
  this->Internals->YAxesBG->AddButton(this->Internals->UseYThresholdBounds);
  this->Internals->YAxesBG->AddButton(this->Internals->UseYCustomScale);

  this->Internals->ZAxesBG->AddButton(this->Internals->UseZFullBounds);
  this->Internals->ZAxesBG->AddButton(this->Internals->UseZThresholdBounds);
  this->Internals->ZAxesBG->AddButton(this->Internals->UseZCustomScale);

  
  //make sure we restore the dialog to have the correct radio buttons
  //activated as it had last time
  if(currentAxesMode[0] == 0 )
    {
    this->Internals->UseXFullBounds.setChecked(true);
    }
  else if (currentAxesMode[0] == 1 )
    {
    this->Internals->UseXThresholdBounds.setChecked(true);
    }
  else
    {
    this->Internals->UseXCustomScale.setChecked(true);
    }

  if(currentAxesMode[1] == 0 )
    {
    this->Internals->UseYFullBounds.setChecked(true);
    }
  else if (currentAxesMode[1] == 1 )
    {
    this->Internals->UseYThresholdBounds.setChecked(true);
    }
  else
    {
    this->Internals->UseYCustomScale.setChecked(true);
    }

  if(currentAxesMode[2] == 0 )
    {
    this->Internals->UseZFullBounds.setChecked(true);
    }
  else if (currentAxesMode[2] == 1 )
    {
    this->Internals->UseZThresholdBounds.setChecked(true);
    }
  else
    {
    this->Internals->UseXCustomScale.setChecked(true);
    }    

  //setup the Full Bounds labels
  const QString BoundsText = QString("%1 - %2");
  this->UseXFullBounds->setText(BoundsText.arg(worldBounds[0],worldBounds[1]));
  this->UseYFullBounds->setText(BoundsText.arg(worldBounds[2],worldBounds[3]));
  this->UseZFullBounds->setText(BoundsText.arg(worldBounds[4],worldBounds[5]));

  //setup the Threshold labels
  this->UseXThresholdBounds->setText(BoundsText.arg(thresholdBounds[0],thresholdBounds[1]));
  this->UseYThresholdBounds->setText(BoundsText.arg(thresholdBounds[2],thresholdBounds[3]));
  this->UseZThresholdBounds->setText(BoundsText.arg(thresholdBounds[4],thresholdBounds[5]));

  //setup the text for the custom scale line edits
  for(int i=0; i < 6; i++)
    {
    this->Internals->CustomBounds[i] = customBounds[i];
    }
  this->Internals->XCustomMin.setText(QString::number(customBounds[0]));
  this->Internals->XCustomMax.setText(QString::number(customBounds[1]));

  this->Internals->YCustomMin.setText(QString::number(customBounds[2]));
  this->Internals->YCustomMax.setText(QString::number(customBounds[3]));

  this->Internals->ZCustomMin.setText(QString::number(customBounds[4]));
  this->Internals->ZCustomMax.setText(QString::number(customBounds[5]));
  
  //setup the custom bounds editing signals
  QObject::connect(this->Internals->XCustomMin, SIGNAL(editingFinished()),
    this, SLOT(onCustomBoundsChanged()));
  QObject::connect(this->Internals->XCustomMax, SIGNAL(editingFinished()),
    this, SLOT(onCustomBoundsChanged()));    
    
  QObject::connect(this->Internals->YCustomMin, SIGNAL(editingFinished()),
    this, SLOT(onCustomBoundsChanged()));
  QObject::connect(this->Internals->YCustomMax, SIGNAL(editingFinished()),
    this, SLOT(onCustomBoundsChanged()));    
    
  QObject::connect(this->Internals->ZCustomMin, SIGNAL(editingFinished()),
    this, SLOT(onCustomBoundsChanged()));
  QObject::connect(this->Internals->ZCustomMax, SIGNAL(editingFinished()),
    this, SLOT(onCustomBoundsChanged()));        
    
  //connect all the radio buttons to the signal mapper
  QObject::connect(this->Internals->UseXFullBounds, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseYFullBounds, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseZFullBounds, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseXThresholdBounds, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseYThresholdBounds, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseZThresholdBounds, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseXCustomScale, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseYCustomScale, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseZCustomScale, SIGNAL(clicked()),
    this->Internals->SigMap, SLOT(map()));

  //setup the signal mapper text string for each button
  this->Internals->SigMap->setMapping(this->Internals->UseXFullBounds, "00");
  this->Internals->SigMap->setMapping(this->Internals->UseXThresholdBounds, "01");
  this->Internals->SigMap->setMapping(this->Internals->UseXCustomScale, "02");
  this->Internals->SigMap->setMapping(this->Internals->UseYFullBounds, "10");
  this->Internals->SigMap->setMapping(this->Internals->UseYThresholdBounds, "11");
  this->Internals->SigMap->setMapping(this->Internals->UseYCustomScale, "12");
  this->Internals->SigMap->setMapping(this->Internals->UseZFullBounds, "20");
  this->Internals->SigMap->setMapping(this->Internals->UseZThresholdBounds, "21");
  this->Internals->SigMap->setMapping(this->Internals->UseZCustomScale, "22");

  QObject::connect(this->Internals->SigMap, SIGNAL(mapped(const QString &)),
  this, SLOT(onModeChanged(const QString &)));
  
}

//-----------------------------------------------------------------------------
PrismScaleViewDialog::~PrismScaleViewDialog()
{
  if(this->Internals)
    {
    delete this->Internals;
    }
  this->Internals = NULL;

  delete this->XAxesBG;
  delete this->YAxesBG;
  delete this->ZAxesBG;
}

//-----------------------------------------------------------------------------
bool PrismScaleViewDialog::hasCustomBounds() const
{
  return (this->ScalingMode[0] == 2 || this->ScalingMode[1] == 2 || this->ScalingMode[2] == 2);
}

//-----------------------------------------------------------------------------
int* PrismScaleViewDialog::scalingMode() const
{
  return this->ScalingMode;
}

//-----------------------------------------------------------------------------
double* PrismScaleViewDialog::customBounds() const
{
  return this->CustomBounds;
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::onModeChanged(const QString& mode)
{
  //convert the string "12" too y axis, custom scale
  this->modeChanged(mode.at(0).digitValue(),mode.at(1).digitValue());
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::onCustomBoundsChanged( )
{
  //ToDo: Find a nicer solution than this
  this->Internals->CustomBounds[0] = this->Internals->XCustomMin.text().toDouble();
  this->Internals->CustomBounds[1] = this->Internals->XCustomMax.text().toDouble();

  this->Internals->CustomBounds[2] = this->Internals->YCustomMin.text().toDouble();
  this->Internals->CustomBounds[3] = this->Internals->YCustomMax.text().toDouble();

  this->Internals->CustomBounds[4] = this->Internals->ZCustomMin.text().toDouble();
  this->Internals->CustomBounds[5] = this->Internals->ZCustomMax.text().toDouble();
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::modeChanged(const int& pos, const int& value)
{
  this->Internals->ScalingMode[pos] = value;
}
