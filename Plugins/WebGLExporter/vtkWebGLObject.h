/*=========================================================================

  Program:   ParaView Web
  Module:    vtkWebGLObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWebGLObject
// .SECTION Description
// vtkWebGLObject represent and manipulate an WebGL object and its data.

#ifndef __vtkWebGLObject_h
#define __vtkWebGLObject_h

class vtkMatrix4x4;

#include "vtkObject.h"
#include <string>

//BTX
enum WebGLObjectTypes {
  wPOINTS = 0,
  wLINES = 1,
  wTRIANGLES = 2
  };
//ETX

class vtkWebGLObject : public vtkObject
{
public:
  static vtkWebGLObject* New();
  vtkTypeMacro(vtkWebGLObject, vtkObject)
  void PrintSelf(ostream &os, vtkIndent indent);

  virtual void GenerateBinaryData();
  virtual unsigned char* GetBinaryData(int part);
  virtual int GetBinarySize(int part);
  virtual int GetNumberOfParts();
//BTX
  void SetLayer(int l);
  void SetRendererId(long int i);
  void SetId(std::string i);
  void SetWireframeMode(bool wireframe);
  void SetVisibility(bool vis);
  void SetTransformationMatrix(vtkMatrix4x4* m);
  void SetIsWidget(bool w);
  void SetHasTransparency(bool t);
  void SetInteractAtServer(bool i);
  void SetType(WebGLObjectTypes t);
  bool isWireframeMode();
  bool isVisible();
  bool HasChanged();
  bool isWidget();
  bool HasTransparency();
  bool InteractAtServer();
  std::string GetMD5();
  std::string GetId();
  long int GetRendererId();
  int GetLayer();

protected:
    vtkWebGLObject();
    ~vtkWebGLObject();

    float Matrix[16];
    long int rendererId;
    int layer;                  // Renderer Layer
    std::string id;          // Id of the object
    std::string MD5;
    bool hasChanged;
    bool iswireframeMode;
    bool isvisible;
    WebGLObjectTypes type;
    bool hasTransparency;
    bool iswidget;
    bool interactAtServer;

private:
  vtkWebGLObject(const vtkWebGLObject&); // Not implemented
  void operator=(const vtkWebGLObject&);   // Not implemented

//  class vtkInternal;
//  vtkInternal* Internal;
//ETX
};

#endif
