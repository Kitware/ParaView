/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWVolumeComposite.h
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
// .NAME vtkKWVolumeComposite - a composite containing a volume
// .SECTION Description
// A composite designed for volumes. Simple version - no GUI


#ifndef __vtkKWVolumeComposite_h
#define __vtkKWVolumeComposite_h

#include "vtkKWComposite.h"

class vtkVolume;
class vtkVolumeProMapper;
class vtkLODProp3D;
class vtkVolumeRayCastCompositeFunction;
class vtkVolumeRayCastMIPFunction;
class vtkVolumeTextureMapper2D;
class vtkImageResample;
class vtkImageData;
class vtkVolumeProperty;
class vtkVolumeRayCastMapper;

class VTK_EXPORT vtkKWVolumeComposite : public vtkKWComposite
{
public:
  static vtkKWVolumeComposite* New();
  vtkTypeMacro(vtkKWVolumeComposite,vtkKWComposite);

  // Description:
  // Set the input data for this volume
  virtual void SetInput(vtkImageData *input);
  virtual vtkImageData *GetInput();

  // Description:
  // Get the prop for this composite
  virtual vtkProp *GetProp() {return (vtkProp *)this->LODVolume;};

  vtkGetObjectMacro( LODVolume, vtkLODProp3D );
  vtkGetMacro( RayCastID, int );
  vtkGetMacro( LowResTextureID, int );
  vtkGetMacro( MedResTextureID, int );
  vtkGetMacro( HiResTextureID, int );

  vtkGetObjectMacro( VolumeProMapper, vtkVolumeProMapper );

  vtkGetObjectMacro( VolumeProperty, vtkVolumeProperty );


protected:
  vtkKWVolumeComposite();
  ~vtkKWVolumeComposite();
  vtkKWVolumeComposite(const vtkKWVolumeComposite&) {};
  void operator=(const vtkKWVolumeComposite&) {};

  vtkLODProp3D                         *LODVolume;
  vtkVolumeProperty                    *VolumeProperty;

  vtkVolumeRayCastCompositeFunction    *Composite;
  vtkVolumeRayCastMIPFunction          *MIP;
  vtkVolumeRayCastMapper               *RayCastMapper;

  vtkImageResample                     *LowResResampler;
  vtkImageResample                     *MedResResampler;

  vtkVolumeTextureMapper2D             *LowResTextureMapper;
  vtkVolumeTextureMapper2D             *MedResTextureMapper;
  vtkVolumeTextureMapper2D             *HiResTextureMapper;
  
  float                                LowResMagnification[3];
  float                                MedResMagnification[3];

  vtkVolumeProMapper                   *VolumeProMapper;
  int                                  UsingVolumeProMapper;

  int                                  RayCastID;
  int                                  LowResTextureID;
  int                                  MedResTextureID;
  int                                  HiResTextureID;
  int                                  VolumeProID;
};


#endif




