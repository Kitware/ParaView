/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWVolumeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWVolumeComposite.h"
#include "vtkKWView.h"
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
#include "vtkObjectFactory.h"
#include "vtkVolume.h"
#include "vtkVolumeProMapper.h"
#include "vtkLODProp3D.h"
#include "vtkKWProgressGauge.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWVolumeComposite );

int vtkKWVolumeCompositeCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

vtkKWVolumeComposite::vtkKWVolumeComposite()
{
  vtkFiniteDifferenceGradientEstimator *gradientEstimator;
  vtkRecursiveSphereDirectionEncoder   *directionEncoder;

  this->Input                 = NULL;
  
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
  this->LowResVolumeProMapper = vtkVolumeProMapper::New();
  this->VProResampler         = vtkImageResample::New();
  
  this->RayCastMapper->IntermixIntersectingGeometryOff();

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
  this->LODVolume->SetLODLevel( this->HiResTextureID, 1.0 );

  this->RayCastID = 
    this->LODVolume->AddLOD( this->RayCastMapper,
			     this->VolumeProperty, 11.0 );
  this->LODVolume->SetLODLevel( this->HiResTextureID, 0.0 );

  this->LowResTextureID   = -1;
  this->MedResTextureID   = -1;
  this->LowResVolumeProID = -1;
  
  this->LowResMagnification[0] = 1.0;
  this->LowResMagnification[1] = 1.0;
  this->LowResMagnification[2] = 1.0;

  this->MedResMagnification[0] = 1.0;
  this->MedResMagnification[1] = 1.0;
  this->MedResMagnification[2] = 1.0;

  this->VProMagnification[0] = 1.0;
  this->VProMagnification[1] = 1.0;
  this->VProMagnification[2] = 1.0;

  // set up the low res and med res mappers to use some storage
  this->LowResTextureMapper->SetMaximumStorageSize(7000000);
  this->MedResTextureMapper->SetMaximumStorageSize(7000000);
  
  vtkPiecewiseFunction *pwf = vtkPiecewiseFunction::New();
  pwf->AddPoint(0,0.0);
  this->VolumeProperty->SetScalarOpacity(pwf);

  vtkColorTransferFunction *ctf = vtkColorTransferFunction::New();
  ctf->AddRGBPoint(0.0,   1.0, 1.0, 1.0);
  ctf->AddRGBPoint(255.0,   1.0, 1.0, 1.0);
  this->VolumeProperty->SetColor(ctf);
  
  this->CommandFunction = vtkKWVolumeCompositeCommand;
  this->CanDoIntermixGeometry = 1;
  this->CanDoHardwareCursor   = 0;
  this->UseIntermixIntersectingGeometry = 0;

  if ( this->VolumeProMapper->GetNumberOfBoards() > 0 )
    {
    if ( this->VolumeProMapper->IsA("vtkOpenGLVolumeProVG500Mapper") )
      {
      this->CanDoIntermixGeometry = 0;
      this->CanDoHardwareCursor   = 1;
      }
    this->VolumeProID = 
      this->LODVolume->AddLOD( this->VolumeProMapper,
                               this->VolumeProperty, 1.0 );
    this->RenderMethod = VTK_VOLUMECOMPOSITE_VOLUMEPRO_METHOD;
    this->VolumeProMapperAvailable = 1;
    this->SoftwareMapperAvailable = 1;
    this->LODVolume->AutomaticLODSelectionOff();
    this->LODVolume->SetSelectedLODID( this->VolumeProID );
    this->VolumeProMapper->IntermixIntersectingGeometryOff();
    }
  else
    {
    this->VolumeProID = -1;
    this->RenderMethod = VTK_VOLUMECOMPOSITE_SOFTWARE_METHOD;
    this->SoftwareMapperAvailable = 1;
    this->VolumeProMapperAvailable = 0;
    }

  pwf->Delete();
  ctf->Delete();
}

vtkKWVolumeComposite::~vtkKWVolumeComposite()
{
  this->Input->UnRegister(this);
  
  this->LODVolume->Delete();
  this->Composite->Delete();
  this->MIP->Delete();
  this->RayCastMapper->Delete();
  this->LowResTextureMapper->Delete();
  this->MedResTextureMapper->Delete();
  this->HiResTextureMapper->Delete();
  this->VolumeProperty->Delete();
  this->LowResVolumeProMapper->Delete();
  
  this->LowResResampler->Delete();
  this->MedResResampler->Delete();

  this->VolumeProMapper->Delete();
  this->VProResampler->Delete();
}

void vtkKWVolumeComposite::SetRenderMethodToSoftware()
{
  if ( this->SoftwareMapperAvailable )
    {
    this->RenderMethod = VTK_VOLUMECOMPOSITE_SOFTWARE_METHOD;
    if ( this->LowResVolumeProID > 0 )
      {
      this->LODVolume->DisableLOD( this->LowResVolumeProID );
      }
    this->LODVolume->DisableLOD( this->VolumeProID );
    this->LODVolume->AutomaticLODSelectionOn();
    }
}

void vtkKWVolumeComposite::SetRenderMethodToVolumePro()
{
  if ( this->VolumeProMapperAvailable )
    {
    this->RenderMethod = VTK_VOLUMECOMPOSITE_VOLUMEPRO_METHOD;
    if ( this->LowResVolumeProID > 0 )
      {
      this->LODVolume->EnableLOD( this->LowResVolumeProID );
      }
    this->LODVolume->EnableLOD( this->VolumeProID );
    this->LODVolume->AutomaticLODSelectionOff();
    this->LODVolume->SetSelectedLODID( this->VolumeProID );
    }
}

void vtkKWVolumeComposite::SetInput(vtkImageData *input)
{
  int   size[3];

  // Hang on to the input
  if ( this->Input )
    {
    this->Input->UnRegister(this);
    }
  this->Input = input;
  if ( input )
    {
    this->Input->Register(this);
    }
  
  // Make sure the user has not turned off Software rendering when Hardware is
  // not available
  if ( !this->VolumeProMapperAvailable )
    {
    this->SoftwareMapperAvailable = 1;
    }

  if ( this->GetView() )
    {
    this->GetView()->GetWindow()->GetProgressGauge()->SetValue(5);
    }

  input->Update();

  if ( this->GetView() )
    {
    this->GetView()->GetWindow()->GetProgressGauge()->SetValue(30);
    }
  
  this->RayCastMapper->SetInput(input);
  this->VolumeProMapper->SetInput(input);
  this->HiResTextureMapper->SetInput(input);

  float *range = input->GetScalarRange();
  float max;

  max = range[1];
  max = (max <= 255.0)?(255.0):(max);
  max = (max > 255.0 && max <= 4095.0)?(4095.0):(max);
  max = (max > 4095.0)?(65535.0):(max);

  float scale = 255.0 / max;
  
  this->RayCastMapper->GetGradientEstimator()->SetInput(input);
  this->RayCastMapper->GetGradientEstimator()->
    SetGradientMagnitudeScale(scale);
  
  if ( this->GetView() )
    {
    this->GetView()->GetWindow()->GetProgressGauge()->SetValue(60);
    }
  
  input->GetDimensions( size );

  if ( this->VolumeProMapperAvailable )
    {
    int subSize[3];
    int i;
    for ( i = 0; i < 3; i++ )
      {
      subSize[i] = (size[i]>256)?(256):(size[i]);
      }
    
    if ( size[0] != subSize[0] ||
         size[1] != subSize[1] ||
         size[2] != subSize[2] )
      {
      this->VProMagnification[0] = (subSize[0] - 0.5) / (float)size[0];
      this->VProMagnification[1] = (subSize[1] - 0.5) / (float)size[1];
      this->VProMagnification[2] = (subSize[2] - 0.5) / (float)size[2];
      this->VProResampler->SetInput(input);
      this->VProResampler->InterpolateOff();
      this->VProResampler->
        SetAxisMagnificationFactor( 0, this->VProMagnification[0] );
      this->VProResampler->
        SetAxisMagnificationFactor( 1, this->VProMagnification[1] );
      this->VProResampler->
        SetAxisMagnificationFactor( 2, this->VProMagnification[2] );
      this->VProResampler->Update();
      this->LowResVolumeProMapper->SetInput( this->VProResampler->GetOutput() );  
      if ( this->GetView() )
        {
        this->GetView()->GetWindow()->GetProgressGauge()->SetValue(70);
        }
      
      this->LowResVolumeProID = 
        this->LODVolume->AddLOD( this->LowResVolumeProMapper,
                                 this->VolumeProperty, 0.0 );
      }
    }
  if ( this->SoftwareMapperAvailable )
    {
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
      this->LowResTextureMapper->GetGradientEstimator()->
        SetInput( this->LowResResampler->GetOutput() );
      this->LowResTextureMapper->GetGradientEstimator()->
        SetGradientMagnitudeScale(scale);
      
      if ( this->GetView() )
        {
        this->GetView()->GetWindow()->GetProgressGauge()->SetValue(70);
        }
      
      this->LowResTextureID = 
        this->LODVolume->AddLOD( this->LowResTextureMapper,
                                 this->VolumeProperty, 0.0 );
      this->LODVolume->SetLODLevel( this->LowResTextureID, 3.0 );
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
      this->MedResTextureMapper->GetGradientEstimator()->
        SetInput( this->MedResResampler->GetOutput() );
      this->MedResTextureMapper->GetGradientEstimator()->
        SetGradientMagnitudeScale(scale);
      
      if ( this->GetView() )
        {
        this->GetView()->GetWindow()->GetProgressGauge()->SetValue(100);
        }
      
      this->MedResTextureID = 
        this->LODVolume->AddLOD( this->MedResTextureMapper,
                                 this->VolumeProperty, 0.0 );
      this->LODVolume->SetLODLevel( this->MedResTextureID, 2.0 );
      }
    }

  
  vtkPiecewiseFunction *pwf = 
    this->VolumeProperty->GetScalarOpacity();
  
  pwf->RemoveAllPoints();
  pwf->AddPoint(  0, 0.0);
  pwf->AddPoint(max, 1.0);
  
  this->VolumeProMapper->GradientOpacityModulationOff();
  this->LowResVolumeProMapper->GradientOpacityModulationOff();

  if ( this->GetView() )
    {
    this->GetView()->GetWindow()->GetProgressGauge()->SetValue(0);
    }
}



vtkImageData *vtkKWVolumeComposite::GetInput()
{
  return this->Input;
}

void vtkKWVolumeComposite::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWComposite::SerializeRevision(os,indent);
  os << indent << "vtkKWVolumeComposite ";
  this->ExtractRevision(os,"$Revision: 1.36 $");
}

vtkProp *vtkKWVolumeComposite::GetProp() 
{
  return static_cast<vtkProp *>(this->LODVolume);
}

void vtkKWVolumeComposite::RegisterIntermixIntersectingGeometry()
{
  if ( !this->UseIntermixIntersectingGeometry )
    {
    this->RayCastMapper->IntermixIntersectingGeometryOn();
    if ( this->CanDoIntermixGeometry )
      {
      this->VolumeProMapper->IntermixIntersectingGeometryOn();
      }
    }
  this->UseIntermixIntersectingGeometry++;
}
void vtkKWVolumeComposite::DeregisterIntermixIntersectingGeometry()
{
  if ( this->UseIntermixIntersectingGeometry <= 0 )
    {
    this->UseIntermixIntersectingGeometry = 0;
    vtkErrorMacro("Reference count for intermix intersecting geometry cannot "
		  "be less than zero.");
    }
  else
    {
    this->UseIntermixIntersectingGeometry--;
    }

  if ( !this->UseIntermixIntersectingGeometry )
    {
    this->RayCastMapper->IntermixIntersectingGeometryOff();
    if ( this->CanDoIntermixGeometry )
      {
      this->VolumeProMapper->IntermixIntersectingGeometryOff();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CanDoHardwareCursor: " << this->GetCanDoHardwareCursor() 
     << endl;
  os << indent << "CanDoIntermixGeometry: " 
     << this->GetCanDoIntermixGeometry() << endl;
  os << indent << "HiResTextureID: " << this->GetHiResTextureID() << endl;
  os << indent << "HiResTextureMapper: " << this->GetHiResTextureMapper() 
     << endl;
  os << indent << "LODVolume: " << this->GetLODVolume() << endl;
  os << indent << "LowResTextureID: " << this->GetLowResTextureID() << endl;
  os << indent << "LowResTextureMapper: " << this->GetLowResTextureMapper() 
     << endl;
  os << indent << "LowResVolumeProID: " << this->GetLowResVolumeProID() 
     << endl;
  os << indent << "MedResTextureID: " << this->GetMedResTextureID() << endl;
  os << indent << "MedResTextureMapper: " << this->GetMedResTextureMapper() 
     << endl;
  os << indent << "RayCastID: " << this->GetRayCastID() << endl;
  os << indent << "RayCastMapper: " << this->GetRayCastMapper() << endl;
  os << indent << "RenderMethod: " << this->GetRenderMethod() << endl;
  os << indent << "SoftwareMapperAvailable: " 
     << this->GetSoftwareMapperAvailable() << endl;
  os << indent << "VolumeProID: " << this->GetVolumeProID() << endl;
  os << indent << "VolumeProMapper: " << this->GetVolumeProMapper() << endl;
  os << indent << "VolumeProMapperAvailable: " 
     << this->GetVolumeProMapperAvailable() << endl;
  os << indent << "VolumeProperty: " << this->GetVolumeProperty() << endl;
}
