/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCornerAnnotation.h
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
// .NAME vtkKWCornerAnnotation - simple annotation for a data set.
// .SECTION Description
// A property class used for annotation of a data set. Provides simple
// functionality such as a bounding box, scalar bar, and labeled axes.

#ifndef __vtkKWCornerAnnotation_h
#define __vtkKWCornerAnnotation_h

#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWGenericComposite.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWView.h"
#include "vtkCornerAnnotation.h"

class VTK_EXPORT vtkKWCornerAnnotation : public vtkKWLabeledFrame
{
public:
  static vtkKWCornerAnnotation* New();
  vtkTypeMacro(vtkKWCornerAnnotation,vtkKWLabeledFrame);

  // Description:
  // Displays and/or updates the property ui display
  virtual void ShowProperties();

  // Description:
  // Create the properties object, called by ShowProperties.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Close out and remove any composites prior to deletion.
  virtual void Close();

  // Description:
  // Set/Get the composite that owns this annotation
  vtkSetObjectMacro(View,vtkKWView);
  vtkGetObjectMacro(View,vtkKWView);

  // Description:
  // Callback functions used by the pro sheet
  virtual void SetCornerText(const char *txt, int corner);
  virtual char *GetCornerText(int i){return this->CornerText[i]->GetValue();};
  virtual void CornerChanged(int i);
  virtual void OnDisplayCorner();
  virtual void SetVisibility(int i);
  virtual int  GetVisibility();
  vtkBooleanMacro(Visibility,int);
  
  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // Change the color of the annotation
  void SetTextColor(float r, float g, float b);
  float *GetTextColor() {return this->CornerProp->GetProperty()->GetColor();};

  // Description:
  // If this corner anno is part of a lightbox, then we'll need some special
  // methods to keep all the corner annotations linked. The ID will indicate
  // if we are part of the lightbox - if it is < 0 we are not, if it is = 0
  // then we are the master lightbox, and the others > 0 are just part of 
  // the lightbox with no visible UI
  vtkSetMacro( LightboxID, int );
  vtkGetMacro( LightboxID, int );

  vtkSetObjectMacro( MasterCornerAnnotation, vtkKWCornerAnnotation );
  vtkGetObjectMacro( MasterCornerAnnotation, vtkKWCornerAnnotation );

  void UpdateFromMaster();

protected:
  vtkKWCornerAnnotation();
  ~vtkKWCornerAnnotation();
  vtkKWCornerAnnotation(const vtkKWCornerAnnotation&) {};
  void operator=(const vtkKWCornerAnnotation&) {};

  vtkKWWidget            *CornerDisplayFrame;
  vtkKWChangeColorButton *CornerColor;
  vtkKWCheckButton       *CornerButton;

  vtkKWWidget            *CornerTopFrame;
  vtkKWWidget            *CornerBottomFrame;

  vtkKWWidget            *CornerFrame[4];
  vtkKWWidget            *CornerLabel[4];
  vtkKWText              *CornerText[4];
  vtkCornerAnnotation    *CornerProp;
  vtkKWGenericComposite  *CornerComposite;

  vtkKWView *View;

  int                    LightboxID;
  vtkKWCornerAnnotation  *MasterCornerAnnotation;
};


#endif



