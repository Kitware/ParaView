/*=========================================================================

  Program:   ParaView Web
  Module:    vtkWebGLWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWebGLWidget
// .SECTION Description
// Widget representation for WebGL.

#ifndef __vtkWebGLWidget_h
#define __vtkWebGLWidget_h

#include "vtkWebGLObject.h"
#include <vector>

class vtkActor2D;

class vtkWebGLWidget : public vtkWebGLObject
{
public:
  static vtkWebGLWidget* New();
  vtkTypeMacro(vtkWebGLWidget, vtkWebGLObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  void GenerateBinaryData();
  unsigned char* GetBinaryData(int part);
  int GetBinarySize(int part);
  int GetNumberOfParts();

  void GetDataFromColorMap(vtkActor2D* actor);

//BTX
protected:
    vtkWebGLWidget();
    ~vtkWebGLWidget();

    unsigned char* binaryData;
    int binarySize;
    int orientation;
    char* title;
    char* textFormat;
    int textPosition;
    float position[2];
    float size[2];
    int numberOfLabels;
    std::vector <double*>colors;      //x, r, g, b

private:
  vtkWebGLWidget(const vtkWebGLWidget&); // Not implemented
  void operator=(const vtkWebGLWidget&); // Not implemented

//ETX
};

#endif
