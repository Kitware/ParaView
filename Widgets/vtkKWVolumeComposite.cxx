/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWVolumeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWVolumeComposite.h"
#include "vtkKWWindow.h"
#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeRayCastMIPFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkLODProp3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkOutlineFilter.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkRecursiveSphereDirectionEncoder.h"
#include "vtkVolumeTextureMapper2D.h"
#include "vtkMath.h"
#include "vtkImageResample.h"
#include "vtkRayCaster.h"
#include "vtkObjectFactory.h"
#include "vtkVolume.h"
#include "vtkVolumeProMapper.h"
#include "vtkLODProp3D.h"


//------------------------------------------------------------------------------
vtkKWVolumeComposite* vtkKWVolumeComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWVolumeComposite");
  if(ret)
    {
    return (vtkKWVolumeComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWVolumeComposite;
}




int vtkKWVolumeCompositeCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

vtkKWVolumeComposite::vtkKWVolumeComposite()
{
  vtkFiniteDifferenceGradientEstimator *gradientEstimator;
  vtkRecursiveSphereDirectionEncoder   *directionEncoder;

  this->LODVolume             = vtkLODProp3D::New();
  this->Composite             = vtkVolumeRayCastCompositeFunction::New();
  this->MIP                   = vtkVolumeRayCastMIPFunction::New();
  this->RayCastMapper         = vtkVolumeRayCastMapper::New();
  this->VolumeProperty        = vtkVolumeProperty::New();
  this->LowResTextureMapper   = vtkVolumeTextureMapper2D::New();
  this->MedResTextureMapper   = vtkVolumeTextureMapper2D::New();
  this->HiResTextureMapper    = vtkVolumeTextureMapper2D::New();
  this->LowResResampler       = vtkImageResample::New();
  this->MedResResampler       = vtkImageResample::New();
  this->VolumeProMapper       = vtkVolumeProMapper::New();

  if ( this->VolumeProMapper->GetNumberOfBoards() > 0 )
    {
    this->LODVolume->SetLODMapper( this->RayCastID, this->VolumeProMapper );
    this->UsingVolumeProMapper = 1;
    this->LODVolume->AutomaticLODSelectionOff();
    this->LODVolume->SetSelectedLODID( this->RayCastID );
    }
  else
    {
    this->UsingVolumeProMapper = 0;
    }

  gradientEstimator = vtkFiniteDifferenceGradientEstimator::New();
  directionEncoder  = vtkRecursiveSphereDirectionEncoder::New();

  gradientEstimator->SetDirectionEncoder( directionEncoder );
  this->RayCastMapper->SetGradientEstimator( gradientEstimator );
  this->HiResTextureMapper->SetGradientEstimator( gradientEstimator );

  gradientEstimator->Delete();
  directionEncoder->Delete();

  this->VolumeProperty->SetInterpolationTypeToLinear();
  this->VolumeProperty->SetAmbient(1.0);
  this->VolumeProperty->SetDiffuse(0.0);
  this->VolumeProperty->SetSpecular(0.0);
  this->VolumeProperty->SetSpecularPower(1.0);
  this->RayCastMapper->SetVolumeRayCastFunction(this->Composite);


  this->HiResTextureID = 
    this->LODVolume->AddLOD( this->HiResTextureMapper,
			     this->VolumeProperty, 0.0 );

  this->RayCastID = 
    this->LODVolume->AddLOD( this->RayCastMapper,
			     this->VolumeProperty, 11.0 );

  this->LowResTextureID = -1;
  this->MedResTextureID = -1;

  this->LowResMagnification[0] = 1.0;
  this->LowResMagnification[1] = 1.0;
  this->LowResMagnification[2] = 1.0;

  this->MedResMagnification[0] = 1.0;
  this->MedResMagnification[1] = 1.0;
  this->MedResMagnification[2] = 1.0;

  vtkPiecewiseFunction *pwf = vtkPiecewiseFunction::New();
  pwf->AddPoint(0,0.0);
  this->VolumeProperty->SetScalarOpacity(pwf);

  vtkColorTransferFunction *ctf = vtkColorTransferFunction::New();
  ctf->AddRGBPoint(0.0,   0.3, 0.3, 0.3);
  this->VolumeProperty->SetColor(ctf);

  this->CommandFunction = vtkKWVolumeCompositeCommand;

  pwf->Delete();
  ctf->Delete();
}

vtkKWVolumeComposite::~vtkKWVolumeComposite()
{
  this->LODVolume->Delete();
  this->Composite->Delete();
  this->MIP->Delete();
  this->RayCastMapper->Delete();
  this->LowResTextureMapper->Delete();
  this->MedResTextureMapper->Delete();
  this->HiResTextureMapper->Delete();
  this->VolumeProperty->Delete();
  
  this->LowResResampler->Delete();
  this->MedResResampler->Delete();

  this->VolumeProMapper->Delete();
}

void vtkKWVolumeComposite::SetInput(vtkImageData *input)
{
  int   size[3];

  input->Update();
  this->RayCastMapper->SetInput(input);
  this->HiResTextureMapper->SetInput(input);

  input->GetDimensions( size );

  // if at least two axes have more than 32 samples
  if ((size[0] > 32) + (size[1] > 32) + (size[2] > 32) > 1)
    {
    this->LowResMagnification[0] = 31.5 / (float)size[0];
    this->LowResMagnification[1] = 31.5 / (float)size[1];
    this->LowResMagnification[2] = 31.5 / (float)size[2];
    this->LowResResampler->SetInput(input);
    this->LowResResampler->InterpolateOff();
    this->LowResResampler->
      SetAxisMagnificationFactor( 0, this->LowResMagnification[0] );
    this->LowResResampler->
      SetAxisMagnificationFactor( 1, this->LowResMagnification[1] );
    this->LowResResampler->
      SetAxisMagnificationFactor( 2, this->LowResMagnification[2] );
    this->LowResResampler->Update();
    this->LowResTextureMapper->SetInput( this->LowResResampler->GetOutput() );
    
    this->LowResTextureID = 
      this->LODVolume->AddLOD( this->LowResTextureMapper,
			       this->VolumeProperty, 0.0 );
    }

  if ( size[0] > 32 && size[1] > 32 && size[2] > 32 &&
       ( size[0] > 64 || size[1] > 64 || size[2] > 64 ) )
    {
    this->MedResMagnification[0] = 63.5 / (float)size[0];
    this->MedResMagnification[1] = 63.5 / (float)size[1];
    this->MedResMagnification[2] = 63.5 / (float)size[2];
    this->MedResResampler->SetInput(input);
    this->MedResResampler->InterpolateOff();
    this->MedResResampler->
      SetAxisMagnificationFactor( 0, this->MedResMagnification[0] );
    this->MedResResampler->
      SetAxisMagnificationFactor( 1, this->MedResMagnification[1] );
    this->MedResResampler->
      SetAxisMagnificationFactor( 2, this->MedResMagnification[2] );
    this->MedResResampler->Update();
    this->MedResTextureMapper->SetInput( this->MedResResampler->GetOutput() );

    this->MedResTextureID = 
      this->LODVolume->AddLOD( this->MedResTextureMapper,
			       this->VolumeProperty, 0.0 );
    }
}

vtkImageData *vtkKWVolumeComposite::GetInput()
{
  return this->RayCastMapper->GetInput();
}
