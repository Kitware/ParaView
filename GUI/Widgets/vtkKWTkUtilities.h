/*=========================================================================

  Module:    vtkKWTkUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTkUtilities - class that supports basic Tk functions
// .SECTION Description
// vtkKWTkUtilities provides methods to perform common Tk operations.

#ifndef __vtkKWTkUtilities_h
#define __vtkKWTkUtilities_h

#include "vtkObject.h"

class vtkImageData;
class vtkKWIcon;
struct Tcl_Interp;

class VTK_EXPORT vtkKWTkUtilities : public vtkObject
{
public:
  static vtkKWTkUtilities* New();
  vtkTypeRevisionMacro(vtkKWTkUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX  

  // Description:
  // Get RGB component for color (given a window).
  static void GetRGBColor(Tcl_Interp *interp,
                          const char *window, 
                          const char *color, 
                          int *rr, int *gg, int *bb);

  // Description:
  // Get option color of window/widget (ex: -bg, -fg, etc.).
  static void GetOptionColor(Tcl_Interp *interp,
                             const char *window, 
                             const char *option, 
                             int *r, int *g, int *b);
  
  // Description:
  // Get background color of window/widget.
  static void GetBackgroundColor(Tcl_Interp *interp,
                                 const char *window, 
                                 int *r, int *g, int *b);

  // Description:
  // Check if a pair of screen coordinates are within the area defined by
  // a window.
  static int ContainsCoordinates(Tcl_Interp *interp,
                                 const char *window, 
                                 int x, int y);
  
  // Description:
  // Update a photo given a pixel structure. 
  // If RGBA (pixel_size > 3), blend pixels with background color of
  // the blend_with_name widget (otherwise 0.5, 0.5, 0.5 gray if NULL).
  // If color_option is not NULL, use this widget option as color instead
  // of background (-bg) (ex: -fg, -selectcolor)
  static int UpdatePhoto(Tcl_Interp *interp,
                         const char *photo_name,
                         const unsigned char *pixels, 
                         int width, int height,
                         int pixel_size,
                         unsigned long buffer_length = 0,
                         const char *blend_with_name = 0,
                         const char *color_option = 0);
  static int UpdatePhoto(Tcl_Interp *interp,
                         const char *photo_name,
                         vtkImageData *image, 
                         const char *blend_with_name = 0,
                         const char *color_option = 0);

  // Description:
  // Update a photo given a pixel structure. 
  // If a file 'file_name.png' is found in 'directory' or 
  // 'directory/Resources' then this file is loaded first, otherwise the
  // remaining parameters will be used to update a photo (see UpdatePhoto)
  // If photo_name is NULL, file_name will be used instead.
  static int UpdateOrLoadPhoto(Tcl_Interp *interp,
                               const char *photo_name,
                               const char *file_name,
                               const char *directory,
                               const unsigned char *pixels, 
                               int width, int height,
                               int pixel_size,
                               unsigned long buffer_length = 0,
                               const char *blend_with_name = 0,
                               const char *color_option = 0);

  // Description:
  // Quick way to get a photo height/width.
  static int GetPhotoHeight(Tcl_Interp *interp,
                            const char *photo_name);
  static int GetPhotoWidth(Tcl_Interp *interp,
                           const char *photo_name);

  // Description:
  // Change the -font weight attribute of widget to bold or normal.
  static int ChangeFontWeightToBold(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontWeightToNormal(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontWeightToBold(Tcl_Interp *interp, const char *widget);
  static int ChangeFontWeightToNormal(Tcl_Interp *interp, const char *widget);

  // Change the -font slant attribute of widget to italic or roman (normal).
  static int ChangeFontSlantToItalic(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontSlantToRoman(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontSlantToItalic(Tcl_Interp *interp, const char *widget);
  static int ChangeFontSlantToRoman(Tcl_Interp *interp, const char *widget);

  // Description:
  // Get the size of a grid (i.e. the number of colums and rows in this 
  // master widget).
  static int GetGridSize(Tcl_Interp *interp,
                         const char *widget,
                         int *nb_of_cols,
                         int *nb_of_rows);

  // Description:
  // Get the position of a widget in a grid.
  static int GetGridPosition(Tcl_Interp *interp,
                             const char *widget,
                             int *col,
                             int *row);

  // Description:
  // Get the bounding box of the slaves of a pack (i.e. the largest width
  // and height of the slaves packed in a master widget, including padding).
  static int GetPackSlavesBbox(Tcl_Interp *interp,
                               const char *widget,
                               int *width,
                               int *height);

  // Description:
  // Get the horizontal position of a slave of a pack (in case 'winfo x' does
  // not work because the widget has not been mapped).
  static int GetPackSlaveHorizontalPosition(Tcl_Interp *interp,
                                            const char *widget,
                                            const char *slave,
                                            int *x);

  // Description:
  // Get the padding info of a slave (packed).
  static int GetPackSlavePadding(Tcl_Interp *interp,
                                 const char *widget,
                                 int *ipadx,
                                 int *ipady,
                                 int *padx,
                                 int *pady);

  // Description:
  // Get the -in value of a slave (packed).
  static int GetPackSlaveIn(Tcl_Interp *interp,
                            const char *widget,
                            ostream &in);

  // Description:
  // Get the column widths of a grid (i.e. a master widget that has been grid).
  // If 'allocate' is true, the resulting array (col_widths) is allocated
  // by the function to match the number of columns.
  // The function iterates over cells to request the width of
  // each slave (winfo reqwidth).
  static int GetGridColumnWidths(Tcl_Interp *interp,
                                 const char *widget,
                                 int *nb_of_cols,
                                 int **col_widths,
                                 int allocate = 0);

  // Description:
  // Synchronize the columns minimum size of different widgets that have
  // been grid. If 'factors' is non-null, it is used as an array of
  // multiplication factor to apply to each column minimum size.
  // If 'weights' is non-null, it is used as an array of weight
  // to apply to each column through columnconfigure -weight.
  static int SynchroniseGridsColumnMinimumSize(Tcl_Interp *interp,
                                               int nb_of_widgets,
                                               const char **widgets,
                                               const float *factors = 0,
                                               const int *weights = 0);

  // Description:
  // Synchronize the width of a set of label widgets. The maximum size is found
  // and assigned to each label. Additionally it will apply the "options" to
  // each widget (if any).
  static int SynchroniseLabelsMaximumWidth(Tcl_Interp *interp,
                                           int nb_of_widgets,
                                           const char **widgets,
                                           const char *options = 0);

  // Description:
  // Store the slaves (packed) of widget in 'slaves'. The slaves array is 
  // allocated automatically and the number of slaves is returned.
  static int GetSlaves(Tcl_Interp *interp,
                       const char *widget,
                       char ***slaves);

  // Description:
  // Browse all the slaves (packed) of widget and store the slave packed
  // before 'slave' in 'previous_slave', after 'slave' in 'next_slave'
  // Return 1 if 'slave' was found, 0 otherwise
  static int GetPreviousAndNextSlave(Tcl_Interp *interp,
                                     const char *widget,
                                     const char *slave,
                                     ostream &previous_slave,
                                     ostream &next_slave);

  //ETX

protected:
  vtkKWTkUtilities() {};
  ~vtkKWTkUtilities() {};

  static int ChangeFontWeight(Tcl_Interp *interp, const char *widget, int);
  static int ChangeFontWeight(Tcl_Interp *interp, 
                              const char *font, char *new_font, int);
  static int ChangeFontSlant(Tcl_Interp *interp, const char *widget, int);
  static int ChangeFontSlant(Tcl_Interp *interp, 
                             const char *font, char *new_font, int);

private:
  vtkKWTkUtilities(const vtkKWTkUtilities&); // Not implemented
  void operator=(const vtkKWTkUtilities&); // Not implemented
};

#endif

