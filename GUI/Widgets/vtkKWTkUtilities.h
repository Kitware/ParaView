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

class vtkKWWidget;
class vtkKWApplication;
struct Tcl_Interp;

class VTK_EXPORT vtkKWTkUtilities : public vtkObject
{
public:
  static vtkKWTkUtilities* New();
  vtkTypeRevisionMacro(vtkKWTkUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the RGB components (0..255) that correspond to 'color' (say, #223344)
  // in the widget given by 'widget' (say, .foo.bar). Color may be specified
  // in any of the forms acceptable for a Tk color option.
  // A convenience method is provided to query a vtkKWWidget directly.
  static void GetRGBColor(Tcl_Interp *interp,
                          const char *widget, 
                          const char *color, 
                          int *r, int *g, int *b);
  static void GetRGBColor(vtkKWWidget *widget, 
                          const char *color, 
                          int *r, int *g, int *b);

  // Description:
  // Get the RGB components (0..255) that correspond to the color 'option'
  // (say -bg, -fg, etc.) of the widget given by 'widget' (say, .foo.bar).
  // A convenience method is provided to query a vtkKWWidget directly.
  static void GetOptionColor(Tcl_Interp *interp,
                             const char *widget, 
                             const char *option, 
                             int *r, int *g, int *b);
  static void GetOptionColor(vtkKWWidget *widget,
                             const char *option, 
                             int *r, int *g, int *b);
  
  // Description:
  // Get the RGB components (0..255) of the background color
  // of the widget given by 'widget' (say, .foo.bar).
  // A convenience method is provided to query a vtkKWWidget directly.
  static void GetBackgroundColor(Tcl_Interp *interp,
                                 const char *widget, 
                                 int *r, int *g, int *b);
  static void GetBackgroundColor(vtkKWWidget *widget,
                                 int *r, int *g, int *b);

  // Description:
  // Check if a pair of screen coordinates (x, y) are within the area defined
  // by the widget given by 'widget' (say, .foo.bar).
  // Return 1 if inside, 0 otherwise.
  // A convenience method is provided to query a vtkKWWidget directly.
  static int ContainsCoordinates(Tcl_Interp *interp,
                                 const char *widget, 
                                 int x, int y);
  static int ContainsCoordinates(vtkKWWidget *widget,
                                 int x, int y);
  
  // Description:
  // Update a Tk photo given by its name 'photo_name' using pixels stored in
  // 'pixels' and structured as a 'width' x 'height' x 'pixel_size' (number
  // of bytes per pixel, 3 for RGB for example).
  // If 'buffer_length' is 0, compute it automatically by multiplying
  // 'pixel_size', 'width' and 'height' together.
  // If RGBA ('pixel_size' > 3), blend the pixels with the background color of
  // the 'blend_with_name' widget (otherwise use a [0.5, 0.5, 0.5] gray).
  // If 'color_option' is not NULL (say -fg or -selectcolor), use this option
  // to retrieve the color instead of using the background color (-bg).
  // If UPDATE_PHOTO_OPTION_FLIP_V is set in 'update_option', flip the image
  // buffer vertically.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  // Return 1 on success, 0 otherwise.
  //BTX
  enum 
  { 
    UPDATE_PHOTO_OPTION_FLIP_V = 1
  };
  //ETX
  static int UpdatePhoto(Tcl_Interp *interp,
                         const char *photo_name,
                         const unsigned char *pixels, 
                         int width, int height,
                         int pixel_size,
                         unsigned long buffer_length = 0,
                         const char *blend_with_name = 0,
                         const char *color_option = 0,
                         int update_options = 0);
  static int UpdatePhoto(vtkKWApplication *app,
                         const char *photo_name,
                         const unsigned char *pixels, 
                         int width, int height,
                         int pixel_size,
                         unsigned long buffer_length = 0,
                         const char *blend_with_name = 0,
                         const char *color_option = 0,
                         int update_options = 0);

  // Description:
  // Update a Tk photo given by its name 'photo_name' using pixels stored in
  // 'pixels' and structured as a 'width' x 'height' x 'pixel_size' (number
  // of bytes per pixel, 3 for RGB for example).
  // If a file 'file_name'.png is found in 'directory' or 
  // 'directory/Resources' then an attempt is made to update the photo using
  // this file. If no file is found, the remaining parameters are used
  // to update the photo by calling UpdatePhoto().
  // As a convenience, if 'photo_name' is NULL, 'file_name' is used instead.
  // Note that only the PNG file format is supported so far (do not provide
  // the .png extension to 'file_name').
  // Return 1 on success, 0 otherwise.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
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
  static int UpdateOrLoadPhoto(vtkKWApplication *app,
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
  // Query if a Tk photo given by its name 'photo_name' exists.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  static int FindPhoto(Tcl_Interp *interp, const char *photo_name);
  static int FindPhoto(vtkKWApplication *app, const char *photo_name);

  // Description:
  // Get the height of a Tk photo given by its name 'photo_name'.
  // If the photo does not exist, return 0 and issue a warning.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  static int GetPhotoHeight(Tcl_Interp *interp, const char *photo_name);
  static int GetPhotoHeight(vtkKWApplication *app, const char *photo_name);

  // Description:
  // Get the width of a Tk photo given by its name 'photo_name'.
  // If the photo does not exist, return 0 and issue a warning.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  static int GetPhotoWidth(Tcl_Interp *interp, const char *photo_name);
  static int GetPhotoWidth(vtkKWApplication *app, const char *photo_name);

  // Description:
  // Change the weight attribute of a Tk font specification given by 'font'.
  // The new font specification is copied to 'new_font'. 
  // It is up to the caller to allocate enough space in 'new_font'.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontWeightToBold(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontWeightToNormal(
    Tcl_Interp *interp, const char *font, char *new_font);

  // Description:
  // Change the weight attribute of a 'widget' -font option.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontWeightToBold(Tcl_Interp *interp, const char *widget);
  static int ChangeFontWeightToBold(vtkKWWidget *widget);
  static int ChangeFontWeightToNormal(Tcl_Interp *interp, const char *widget);
  static int ChangeFontWeightToNormal(vtkKWWidget *widget);

  // Description:
  // Change the slant attribute of a Tk font specification given by 'font'.
  // The new font specification is copied to 'new_font'. 
  // It is up to the caller to allocate enough space in 'new_font'.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontSlantToItalic(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontSlantToRoman(
    Tcl_Interp *interp, const char *font, char *new_font);

  // Description:
  // Change the slant attribute of a 'widget' -font option.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontSlantToItalic(Tcl_Interp *interp, const char *widget);
  static int ChangeFontSlantToItalic(vtkKWWidget *widget);
  static int ChangeFontSlantToRoman(Tcl_Interp *interp, const char *widget);
  static int ChangeFontSlantToRoman(vtkKWWidget *widget);

  // Description:
  // Get the number of colums and rows defined in the grid layout of
  // the widget given by 'widget' (say, .foo.bar).
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetGridSize(Tcl_Interp *interp,
                         const char *widget,
                         int *nb_of_cols,
                         int *nb_of_rows);
  static int GetGridSize(vtkKWWidget *widget,
                         int *nb_of_cols,
                         int *nb_of_rows);

  // Description:
  // Get the grid position (column, row) of the widget given by 'widget'
  // (say, .foo.bar).
  // We assume that the current widget layout is a Tk grid.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetPositionInGrid(Tcl_Interp *interp,
                                     const char *widget,
                                     int *col,
                                     int *row);
  static int GetWidgetPositionInGrid(vtkKWWidget *widget,
                                     int *col,
                                     int *row);

  // Description:
  // Get the bounding box size (width, height) of the slaves packed in the
  // widget given by 'widget' (say, .foo.bar), i.e. the largest width
  // and height of the slaves packed in the widget, including padding options.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetSlavesBoundingBoxInPack(Tcl_Interp *interp,
                                        const char *widget,
                                        int *width,
                                        int *height);
  static int GetSlavesBoundingBoxInPack(vtkKWWidget *widget,
                                        int *width,
                                        int *height);

  // Description:
  // Get the horizontal position 'x' in pixels of a slave widget given by
  // 'slave' (say .foo.bar.sl) in the widget given by 'widget' (say .foo.bar).
  // This can be used in case 'winfo x' does not work because the widget
  // has not been mapped yet.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query vtkKWWidget(s) directly.
  // Return 1 on success, 0 otherwise.
  static int GetSlaveHorizontalPositionInPack(Tcl_Interp *interp,
                                              const char *widget,
                                              const char *slave,
                                              int *x);
  static int GetSlaveHorizontalPositionInPack(vtkKWWidget *widget,
                                              vtkKWWidget *slave,
                                              int *x);

  // Description:
  // Get the padding values of the widget given by 'widget' (say .foo.bar)
  // in its layout.
  // We assume that the current widget layout is a Tk pack.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetPaddingInPack(Tcl_Interp *interp,
                                    const char *widget,
                                    int *ipadx,
                                    int *ipady,
                                    int *padx,
                                    int *pady);

  // Description:
  // Get the container a widget given by 'widget' (say .foo.bar) is packed in.
  // This is similar to the Tk -in pack option.
  // Write the container widget name to the output stream 'in'.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetMasterInPack(Tcl_Interp *interp,
                             const char *widget,
                             ostream &in);
  static int GetMasterInPack(vtkKWWidget *widget,
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
  // Synchronize the width of a set of labels given by an array
  // of 'nb_of_widgets' widgets stored in 'widgets'. The maximum size of
  // the labels is found and assigned to each label. 
  // Additionally it will apply the 'options' to/ each widget (if any).
  // A convenience method is provided to specify the vtkKWApplication these
  // widgets belongs to, instead of the Tcl interpreter.
  // Return 1 on success, 0 otherwise.
  static int SynchroniseLabelsMaximumWidth(Tcl_Interp *interp,
                                           int nb_of_widgets,
                                           const char **widgets,
                                           const char *options = 0);
  static int SynchroniseLabelsMaximumWidth(vtkKWApplication *app,
                                           int nb_of_widgets,
                                           const char **widgets,
                                           const char *options = 0);

  // Description:
  // Store the slaves packed in the widget given by 'widget' (say, .foo.bar)
  // in the array 'slaves'. This array is  allocated automatically.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return the number of slaves.
  static int GetSlavesInPack(Tcl_Interp *interp,
                             const char *widget,
                             char ***slaves);
  static int GetSlavesInPack(vtkKWWidget *widget,
                             char ***slaves);

  // Description:
  // Browse all the slaves of the widget given by 'widget' (say, .foo.bar)
  // and store the slave packed before 'slave' in 'previous_slave', and the
  // slave packed after 'slave' in 'next_slave'
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 if 'slave' was found, 0 otherwise
  static int GetPreviousAndNextSlaveInPack(Tcl_Interp *interp,
                                           const char *widget,
                                           const char *slave,
                                           ostream &previous_slave,
                                           ostream &next_slave);
  static int GetPreviousAndNextSlaveInPack(vtkKWWidget *widget,
                                           vtkKWWidget *slave,
                                           ostream &previous_slave,
                                           ostream &next_slave);
  // Description:
  // Take screendump of the widget given by 'widget' (say, .foo.bar) and store
  // it into a png file given by 'fname'.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int TakeScreenDump(Tcl_Interp *interp,
                            const char *wname, 
                            const char *fname, 
                            int top = 0, int bottom = 0, 
                            int left = 0, int right = 0);
  static int TakeScreenDump(vtkKWWidget *widget,
                            const char *fname, 
                            int top = 0, int bottom = 0, 
                            int left = 0, int right = 0);

protected:
  vtkKWTkUtilities() {};
  ~vtkKWTkUtilities() {};

  //BTX  
  //ETX

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

