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


  gradientEstimator = vtkFiniteDifferenceGradientEstimator::New();
  directionEncoder  = vtkRecursiveSphereDirectionEncoder::New();

  gradientEstimator->SetDirectionEncoder( directionEncoder );
  gradientEstimator->ZeroPadOff();
  this->RayCastMapper->SetGradientEstimator( gradientEstimator );
  this->HiResTextureMapper->SetGradientEstimator( gradientEstimator );

  gradientEstimator->Delete();
  directionEncoder->Delete();

  this->VolumeProperty->SetInterpolationTypeToLinear();
  this->VolumeProperty->SetAmbient(1.0);
  this->VolumeProperty->SetDiffuse(0.0);
  this->VolumeProperty->SetSpecular(0.0);
  this->VolumeProperty->SetSpecularPower(1.0);
  this->VolumeProperty->ShadeOff();
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
  ctf->AddRGBPoint(0.0,   1.0, 1.0, 1.0);
  ctf->AddRGBPoint(255.0,   1.0, 1.0, 1.0);
  this->VolumeProperty->SetColor(ctf);

  this->CommandFunction = vtkKWVolumeCompositeCommand;

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
  this->VolumeProMapper->SetInput(input);
  this->HiResTextureMapper->SetInput(input);

  input->GetDimensions( size );

  // if there is an opportunity for a 1/16th size volume then do it
  // we find the target volume size and then reduce it
  int subSize[3];
  int i;
  for (i = 0; i < 3; i++)
    {
    subSize[i] = 2;
    while (subSize[i] < size[i])
      {
      subSize[i] = subSize[i] * 2;
      }
    }
  float ratio = 1.0;
  float numVoxels = subSize[0]*subSize[1]*subSize[2];
  float numTVoxels = numVoxels;
  while ((ratio > 1/16.1 || numVoxels > 66000.0) && 
         subSize[0] > 1 && subSize[1] > 1 && subSize[2] > 1)
    {
    // find the largest axis and divide it by 2
    if (subSize[0] > subSize[1])
      {
      if (subSize[0] > subSize[2])
        {
        subSize[0] = subSize[0] / 2;
        }
      else
        {
        subSize[2] = subSize[2] / 2;
        }
      }
    else
      {
      if (subSize[1] > subSize[2])
        {
        subSize[1] = subSize[1] / 2;
        }
      else
        {
        subSize[2] = subSize[2] / 2;
        }
      }
    numVoxels = subSize[0]*subSize[1]*subSize[2];
    ratio = numVoxels / numTVoxels;
    }
  // now we have a new volume at least 1 16th as small as the original, 
  // the question is, is it large enough to bother with?
  if (numVoxels > 32000)
    {
    this->LowResMagnification[0] = (subSize[0] - 0.5) / (float)size[0];
    this->LowResMagnification[1] = (subSize[1] - 0.5) / (float)size[1];
    this->LowResMagnification[2] = (subSize[2] - 0.5) / (float)size[2];
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
    
    this->LowResTextureMapper->GetGradientEstimator()->ZeroPadOff();
    
    this->LowResTextureID = 
      this->LODVolume->AddLOD( this->LowResTextureMapper,
			       this->VolumeProperty, 0.0 );
    }

  // if there is an opportunity for a 1/4th size volume then do it
  // we find the target volume size and then reduce it
  for (i = 0; i < 3; i++)
    {
    subSize[i] = 2;
    while (subSize[i] < size[i])
      {
      subSize[i] = subSize[i] * 2;
      }
    }
  ratio = 1.0;
  numVoxels = subSize[0]*subSize[1]*subSize[2];
  numTVoxels = numVoxels;
  while ((ratio > 1/4.01 || numVoxels > 263000.0 ) && 
         subSize[0] > 1 && subSize[1] > 1 && subSize[2] > 1)
    {
    // find the largest axis and divide it by 2
    if (subSize[0] > subSize[1])
      {
      if (subSize[0] > subSize[2])
        {
        subSize[0] = subSize[0] / 2;
        }
      else
        {
        subSize[2] = subSize[2] / 2;
        }
      }
    else
      {
      if (subSize[1] > subSize[2])
        {
        subSize[1] = subSize[1] / 2;
        }
      else
        {
        subSize[2] = subSize[2] / 2;
        }
      }
    numVoxels = subSize[0]*subSize[1]*subSize[2];
    ratio = numVoxels / numTVoxels;
    }
  // now we have a new volume at least 1 4th as small as the original, 
  // the question is, is it large enough to bother with?
  if (numVoxels > 32000)
    {
    this->MedResMagnification[0] = (subSize[0] - 0.5) / (float)size[0];
    this->MedResMagnification[1] = (subSize[1] - 0.5) / (float)size[1];
    this->MedResMagnification[2] = (subSize[2] - 0.5) / (float)size[2];
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

    this->MedResTextureMapper->GetGradientEstimator()->ZeroPadOff();
    
    this->MedResTextureID = 
      this->LODVolume->AddLOD( this->MedResTextureMapper,
			       this->VolumeProperty, 0.0 );
    }
  vtkPiecewiseFunction *pwf = 
    this->VolumeProperty->GetScalarOpacity();
  
  float *range = input->GetScalarRange();
  float max;

  max = range[1];
  max = (max <= 255.0)?(255.0):(max);
  max = (max > 255.0 && max <= 4095.0)?(4095.0):(max);
  max = (max > 4095.0)?(65535.0):(max);
  
  pwf->RemoveAllPoints();
  pwf->AddPoint(  0, 0.0);
  pwf->AddPoint(max, 1.0);
  
  pwf = vtkPiecewiseFunction::New();
  pwf->AddPoint(         0.0, 0.0);
  pwf->AddPoint(         1.0, 0.0);
  pwf->AddPoint( (max/ 50.0), 1.0);
  pwf->AddPoint(         max, 1.0);
  this->VolumeProperty->SetGradientOpacity(pwf);
  pwf->Delete();
  this->VolumeProMapper->GradientOpacityModulationOn();
}



vtkImageData *vtkKWVolumeComposite::GetInput()
{
  return this->RayCastMapper->GetInput();
}

void vtkKWVolumeComposite::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWComposite::SerializeRevision(os,indent);
  os << indent << "vtkKWVolumeComposite ";
  this->ExtractRevision(os,"$Revision: 1.6 $");
}
