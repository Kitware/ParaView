/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GeneratePVColorSeries.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkColor.h"
#include "vtkColorSeries.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"

#include <cstdio> // For EXIT_SUCCESS

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// This program generates discrete color maps to be inserted
// into the Qt/Components/Resources/XML/ColorMaps.xml file.
int main(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int valResult = 1;

  VTK_CREATE(vtkColorSeries, palettes);
  vtkColor3ub color;

  cout.precision(16);
  int np = palettes->GetNumberOfColorSchemes();
  for (int p = 0; p < np; ++p)
  {
    palettes->SetColorScheme(p);
    int nc = palettes->GetNumberOfColors(); // in the current scheme
    cout << "  <ColorMap name=\"" << palettes->GetColorSchemeName()
         << "\" space=\"HSV\" indexedLookup=\"true\">\n";
    double x;
    for (int c = 0; c < nc; ++c)
    {
      x = c / (nc - 1.);
      color = palettes->GetColorRepeating(c);
      cout << "    <Point"
           << " x=\"" << x << "\""
           << " r=\"" << color.GetRed() / 255. << "\""
           << " g=\"" << color.GetGreen() / 255. << "\""
           << " b=\"" << color.GetBlue() / 255. << "\""
           << " o=\"1.0\"/>\n";
    }
    cout << "    <NaN"
         << " r=\"" << color.GetRed() / 255. << "\""
         << " g=\"" << color.GetGreen() / 255. << "\""
         << " b=\"" << color.GetBlue() / 255. << "\""
         << "/>\n";
    cout << "  </ColorMap>\n";
  }

  return valResult ? EXIT_SUCCESS : EXIT_FAILURE;
}
