/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVApplication.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkKWDialog.h"
#include "vtkKWWindowCollection.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkKWMessageDialog.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"



//---------------------------------------------------------------------------
vtkPVApplication* vtkPVApplication::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVApplication");
  if(ret)
    {
    return (vtkPVApplication*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVApplication;
}

vtkPVApplication::vtkPVApplication()
{
  this->SetApplicationName("ParaView");
}

int vtkPVApplication::AcceptLicense()
{
  return 1;
}

int vtkPVApplication::AcceptEvaluation()
{
  return 1;
}

#ifdef _WIN32
void ReadAValue(HKEY hKey,char *val,char *key, char *adefault)
{
  DWORD dwType, dwSize;
  
  dwType = REG_SZ;
  dwSize = 40;
  if(RegQueryValueEx(hKey,key, NULL, &dwType, 
                     (BYTE *)val, &dwSize) != ERROR_SUCCESS)
    {
    strcpy(val,adefault);
    }
}
#endif

int VerifyKey(unsigned long key, const char *name, int id)
{
  // find letters
  int letters[80];
  int numLetters = 0;
  unsigned long pos = 0;
  
  // extract letters and convert to numbers
  while (pos < strlen(name))
    {
    if (name[pos] >= 'A' && name[pos] <= 'Z')
      {
      letters[numLetters] = name[pos] - 'A';
      numLetters++;
      }
    if (name[pos] >= 'a' && name[pos] <= 'z')
      {
      letters[numLetters] = name[pos] - 'a';
      numLetters++;
      }
    pos++;
    }

  if (numLetters < 1)
    {
    return 0;
    }
  
  // now assign numbers into value
  unsigned long value = 0;
  pos = 29%numLetters;
  int slot1 = letters[pos];
  pos = (pos + 29)%numLetters;
  int slot2 = letters[pos];
  pos = (pos + 29)%numLetters;
  int slot3 = letters[pos];
  pos = (pos + 29)%numLetters;
  int slot4 = letters[pos];
  pos = (pos + 29)%numLetters;
  int slot5 = letters[pos];

  value = slot1;
  value = value << 6;
  value = value + slot2;
  value = value << 6;
  value = value + id;
  value = value << 5;
  value = value + slot3;
  value = value << 5;
  value = value + slot4;
  value = value << 5;
  value = value + slot5;

  if (value == (key & 0xFBFFFFFF))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

int vtkPVApplication::PromptRegistration(char *name, char *IDS)
{
  return 1;
}

int vtkPVApplication::CheckRegistration()
{
  return 1;
}

void vtkPVApplication::Start(int argc, char*argv[])
{
  // is this copy registered ?
  if (!this->CheckRegistration())
    {
    return;
    }  
  
  vtkPVWindow *ui = vtkPVWindow::New();
  this->Windows->AddItem(ui);
  ui->Create(this,"");
  
  if (argc > 1)
    {
    // if a tcl script was passed in as an arg then load it
    if (!strcmp(argv[1] + strlen(argv[1]) - 4,".tcl"))
      {
      ui->LoadScript(argv[1]);
      }
    // otherwise try to load it as a volume
    else
      {
      if (strlen(argv[1]) > 1)
        {
        ui->Open(argv[1]);
        }
      }
    }

  ui->Delete();
  this->vtkKWApplication::Start(argc,argv);
}

void vtkPVApplication::DisplayAbout(vtkKWWindow *win)
{
  
  if (!this->AcceptLicense())
    {
    this->Exit();
    }
}
