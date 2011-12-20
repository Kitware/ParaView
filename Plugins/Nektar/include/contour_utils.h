
#include <stdio.h>
#include <math.h>
#include <veclib.h>



// Functions defined in contour_utils.C


#ifdef MAP
void iso_contour(Element_List **U, Map *mapx, Map *mapy,  int nfields, FILE *out);
#else
void iso_contour(Element_List **U,  int nfields, FILE *out);
#endif
