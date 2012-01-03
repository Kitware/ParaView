/*=========================================================================

Program: ParaView
Module:    PrismScaleViewDialog.cxx

=========================================================================*/
#include "PrismScaleViewDialog.h"
#include "ui_PrismViewScalingWidget.h"

#include "PrismView.h"

#include <QRadioButton>
#include <QButtonGroup>
#include <QSignalMapper>
#include <QCloseEvent>

#include "pqSettings.h"
#include "pqApplicationCore.h"

class PrismScaleViewDialog::pqInternals : public Ui::PrismViewScalingWidget
{
public:
  QButtonGroup XAxesBG;
  QButtonGroup YAxesBG;
  QButtonGroup ZAxesBG;

  QSignalMapper SigMap;

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
  : Superclass(parentObject, _flags)
{
  this->Internals = new pqInternals();

  this->Internals->setupUi(this);

  //Setup the button groups so we have a group for each axes
  this->Internals->XAxesBG.addButton(this->Internals->UseXFullBounds);
  this->Internals->XAxesBG.addButton(this->Internals->UseXThresholdBounds);
  this->Internals->XAxesBG.addButton(this->Internals->UseXCustomScale);

  this->Internals->YAxesBG.addButton(this->Internals->UseYFullBounds);
  this->Internals->YAxesBG.addButton(this->Internals->UseYThresholdBounds);
  this->Internals->YAxesBG.addButton(this->Internals->UseYCustomScale);

  this->Internals->ZAxesBG.addButton(this->Internals->UseZFullBounds);
  this->Internals->ZAxesBG.addButton(this->Internals->UseZThresholdBounds);
  this->Internals->ZAxesBG.addButton(this->Internals->UseZCustomScale);

  //setup the custom bounds editing signals
  QObject::connect(this->Internals->XCustomMin, SIGNAL(textChanged(QString)),
    this, SLOT(onCustomBoundsChanged()));
  QObject::connect(this->Internals->XCustomMax, SIGNAL(textChanged(QString)),
    this, SLOT(onCustomBoundsChanged()));

  QObject::connect(this->Internals->YCustomMin, SIGNAL(textChanged(QString)),
    this, SLOT(onCustomBoundsChanged()));
  QObject::connect(this->Internals->YCustomMax, SIGNAL(textChanged(QString)),
    this, SLOT(onCustomBoundsChanged()));

  QObject::connect(this->Internals->ZCustomMin, SIGNAL(textChanged(QString)),
    this, SLOT(onCustomBoundsChanged()));
  QObject::connect(this->Internals->ZCustomMax, SIGNAL(textChanged(QString)),
    this, SLOT(onCustomBoundsChanged()));

  //connect all the radio buttons to the signal mapper
  QObject::connect(this->Internals->UseXFullBounds, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseYFullBounds, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseZFullBounds, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseXThresholdBounds, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseYThresholdBounds, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseZThresholdBounds, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseXCustomScale, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseYCustomScale, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));
  QObject::connect(this->Internals->UseZCustomScale, SIGNAL(clicked()),
    &this->Internals->SigMap, SLOT(map()));

  //setup the signal mapper text string for each button
  this->Internals->SigMap.setMapping(this->Internals->UseXFullBounds, "00");
  this->Internals->SigMap.setMapping(this->Internals->UseXThresholdBounds, "01");
  this->Internals->SigMap.setMapping(this->Internals->UseXCustomScale, "02");
  this->Internals->SigMap.setMapping(this->Internals->UseYFullBounds, "10");
  this->Internals->SigMap.setMapping(this->Internals->UseYThresholdBounds, "11");
  this->Internals->SigMap.setMapping(this->Internals->UseYCustomScale, "12");
  this->Internals->SigMap.setMapping(this->Internals->UseZFullBounds, "20");
  this->Internals->SigMap.setMapping(this->Internals->UseZThresholdBounds, "21");
  this->Internals->SigMap.setMapping(this->Internals->UseZCustomScale, "22");

  QObject::connect(&this->Internals->SigMap, SIGNAL(mapped(const QString &)),
  this, SLOT(onModeChanged(const QString &)));

  //connect the button box up
  QObject::connect(this->Internals->buttonBox, SIGNAL(clicked(QAbstractButton*)),
    this, SLOT(onButtonClicked(QAbstractButton*)));
}

//-----------------------------------------------------------------------------
PrismScaleViewDialog::~PrismScaleViewDialog()
{
  if(this->Internals)
    {
    delete this->Internals;
    }
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::setView(PrismView* view)
{
  if ( view && this->View != view )
    {
    this->View = view;    
    }
  if (this->View)
    {
    this->setupViewInfo();
    }
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::setupViewInfo( )
{
  double worldBounds[6], thresholdBounds[6], customBounds[6];
  this->View->GetWorldBounds(worldBounds);
  this->View->GetThresholdBounds(thresholdBounds);
  this->View->GetCustomBounds(customBounds);

  int currentAxesMode[3];
  this->View->GetWorldScaleMode(currentAxesMode);
  
  //make sure we restore the dialog to have the correct radio buttons
  //activated as it had last time. Ugghh todo make this cleaner
  if(currentAxesMode[0] == 0 )
    {
    this->Internals->UseXFullBounds->setChecked(true);
    }
  else if (currentAxesMode[0] == 1 )
    {
    this->Internals->UseXThresholdBounds->setChecked(true);
    }
  else
    {
    this->Internals->UseXCustomScale->setChecked(true);
    }

  if(currentAxesMode[1] == 0 )
    {
    this->Internals->UseYFullBounds->setChecked(true);
    }
  else if (currentAxesMode[1] == 1 )
    {
    this->Internals->UseYThresholdBounds->setChecked(true);
    }
  else
    {
    this->Internals->UseYCustomScale->setChecked(true);
    }

  if(currentAxesMode[2] == 0 )
    {
    this->Internals->UseZFullBounds->setChecked(true);
    }
  else if (currentAxesMode[2] == 1 )
    {
    this->Internals->UseZThresholdBounds->setChecked(true);
    }
  else
    {
    this->Internals->UseXCustomScale->setChecked(true);
    }

  //setup the Full Bounds labels
  const QString BoundsText = QString("%1 - %2");
  this->Internals->UseXFullBounds->setText(BoundsText.arg(
    QString::number(worldBounds[0]),QString::number(worldBounds[1])));
  this->Internals->UseYFullBounds->setText(BoundsText.arg(
    QString::number(worldBounds[2]),QString::number(worldBounds[3])));
  this->Internals->UseZFullBounds->setText(BoundsText.arg(
    QString::number(worldBounds[4]),QString::number(worldBounds[5])));

  //setup the Threshold labels
  this->Internals->UseXThresholdBounds->setText(BoundsText.arg(
    QString::number(thresholdBounds[0]),QString::number(thresholdBounds[1])));
  this->Internals->UseYThresholdBounds->setText(BoundsText.arg(
    QString::number(thresholdBounds[2]),QString::number(thresholdBounds[3])));
  this->Internals->UseZThresholdBounds->setText(BoundsText.arg(
    QString::number(thresholdBounds[4]),QString::number(thresholdBounds[5])));

  //setup the text for the custom scale line edits
  for(int i=0; i < 6; i++)
    {
    this->Internals->CustomBounds[i] = customBounds[i];
    }
  this->Internals->XCustomMin->setText(QString::number(customBounds[0]));
  this->Internals->XCustomMax->setText(QString::number(customBounds[1]));

  this->Internals->YCustomMin->setText(QString::number(customBounds[2]));
  this->Internals->YCustomMax->setText(QString::number(customBounds[3]));

  this->Internals->ZCustomMin->setText(QString::number(customBounds[4]));
  this->Internals->ZCustomMax->setText(QString::number(customBounds[5]));
}

//-----------------------------------------------------------------------------
bool PrismScaleViewDialog::hasCustomBounds() const
{
  return (this->Internals->ScalingMode[0] == 2 ||
          this->Internals->ScalingMode[1] == 2 ||
          this->Internals->ScalingMode[2] == 2);
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
  this->Internals->CustomBounds[0] = this->Internals->XCustomMin->text().toDouble();
  this->Internals->CustomBounds[1] = this->Internals->XCustomMax->text().toDouble();

  this->Internals->CustomBounds[2] = this->Internals->YCustomMin->text().toDouble();
  this->Internals->CustomBounds[3] = this->Internals->YCustomMax->text().toDouble();

  this->Internals->CustomBounds[4] = this->Internals->ZCustomMin->text().toDouble();
  this->Internals->CustomBounds[5] = this->Internals->ZCustomMax->text().toDouble();
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::modeChanged(const int& position, const int& value)
{
  this->Internals->ScalingMode[position] = value;
}
//-----------------------------------------------------------------------------
void PrismScaleViewDialog::onButtonClicked(QAbstractButton* button)
  {
  QDialogButtonBox::ButtonRole role = 
    this->Internals->buttonBox->buttonRole(button);
  switch(role)
    {
    case QDialogButtonBox::ApplyRole:
      this->updateView();
      break;
    case QDialogButtonBox::AcceptRole:
      this->updateView();
      this->accept();
      break;
    case QDialogButtonBox::RejectRole:
    default:
      this->reject();
      break;
    }
  this->saveWindowPosition();
  }

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::updateView()
{
  //push the changes in the scaling down to the view
  this->View->SetWorldScaleMode(this->Internals->ScalingMode);
  if (this->hasCustomBounds())
    {
    this->View->SetCustomBounds(this->Internals->CustomBounds);
    }
  this->View->render();
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::closeEvent(QCloseEvent* cevent)
{
  this->saveWindowPosition();
  cevent->accept();
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::saveWindowPosition()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("PrismPlugin/ViewScaleDialog/geometry",
        this->saveGeometry());
}

//-----------------------------------------------------------------------------
void PrismScaleViewDialog::show( )
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  this->restoreGeometry(
    settings->value("PrismPlugin/ViewScaleDialog/geometry").toByteArray());
  Superclass::show();
}
