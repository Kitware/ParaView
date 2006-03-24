/*=========================================================================

  Module:    vtkKWInternationalization.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkKWInternationalization_h
#define __vtkKWInternationalization_h

#include "vtkObject.h"
#include "vtkKWWidgets.h"  // Needed for export symbols directives

/* ----------------------------------------------------------------------
   If Internationalization is supported, provide simple macros to 
   map gettext function calls.
   Some macros rely on the GETTEXT_DOMAIN symbol to be defined as the quoted
   name of the text domain that applies to the compilation unit. 
   This is done automatically by the KWWidgets_CREATE_GETTEXT_TARGETS
   macro (KWWidgetsInternationalizationMacros.cmake) so that the below macros
   can be used directly and expand to the right text domain automatically.
 */

#ifdef KWWidgets_USE_INTERNATIONALIZATION
#include <libintl.h>  // Bring gettext
#define _(string) gettext(string)
#define k_(string) dgettext(GETTEXT_DOMAIN, string)
#else
#define _(string) string
#define k_(string) string
#define gettext(string) string
#define dgettext(domain,string) string
#ifndef GETTEXT_DOMAIN
#define GETTEXT_DOMAIN ""
#endif
#endif

#define gettext_noop(string) string
#define N_(string) gettext_noop(string)

/* ----------------------------------------------------------------------
   Declare some gettext functions that support enlengthen strings. 
   Enlengthen strings make use of a special *separator* as a mean
   to provide more context and disambiguate short GUI strings.
   Example: "Menu|File|Open" instead of "Open"
   See gettext "10.2.6 How to use gettext in GUI programs".
*/

//BTX
KWWidgets_EXTERN KWWidgets_EXPORT char* kww_sgettext(const char *msgid);
KWWidgets_EXTERN KWWidgets_EXPORT char* kww_sdgettext(const char *domain_name, const char *msgid);
//ETX

#define s_(string) kww_sgettext(string)
#define ks_(string) kww_sdgettext(GETTEXT_DOMAIN, string)

/* ---------------------------------------------------------------------- */

class KWWidgets_EXPORT vtkKWInternationalization : public vtkObject
{
public:
  static vtkKWInternationalization* New();
  vtkTypeRevisionMacro(vtkKWInternationalization,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the current global domain of the LC_MESSAGES category. 
  // This domain specifies where the internationalized/translated strings
  // are coming from.
  // The argument is a null-terminated string, whose characters must be legal
  // in the use in filenames. 
  static void SetCurrentTextDomain(const char *domain_name);
  static const char* GetCurrentTextDomain();

  // Description:
  // Set/Get the binding between a domain and a message catalog directory.
  // The directory should be top-most directory containing the sub-directories
  // for each locale/language (ex: fr, zh_CN). Each language subdirectory has 
  // a LC_MESSAGES subdirectory where the message catalog for that specific
  // domain can be found (ex: dir/fr/LC_MESSAGES/myapp.mo).
  static void SetTextDomainBinding(const char *domain_name, const char *dir);
  static const char* GetTextDomainBinding(const char *domain_name);

  // Description:
  // This method tries *really* hard to find *and* set a message catalog 
  // directory for a specific domain. 
  // Another signature accepts a semi-colon separated list of directories
  // to search message catalogs for.
  // Returns catalog directory if it was found, NULL otherwise.
  static const char* FindTextDomainBinding(const char *domain_name);
  static const char* FindTextDomainBinding(
    const char *domain_name, const char *dirs_to_search);

protected:
  vtkKWInternationalization() {};
  ~vtkKWInternationalization() {};

private:
  vtkKWInternationalization(const vtkKWInternationalization&); // Not implemented
  void operator=(const vtkKWInternationalization&); // Not implemented
};

#endif
