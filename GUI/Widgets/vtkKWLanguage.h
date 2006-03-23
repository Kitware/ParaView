/*=========================================================================

  Module:    vtkKWLanguage.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLanguage - language support.
// .SECTION Description
// vtkKWLanguage provides methods to refer to common languages, as well
// as set the current language.

#ifndef __vtkKWLanguage_h
#define __vtkKWLanguage_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class KWWidgets_EXPORT vtkKWLanguage : public vtkObject
{
public:
  static vtkKWLanguage* New();
  vtkTypeRevisionMacro(vtkKWLanguage,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // List of languages IDs.
  //BTX
  enum
  {
    ABKHAZIAN = 0,
    AFAR,
    AFRIKAANS,
    ALBANIAN,
    AMHARIC,
    ARABIC,
    ARABIC_ALGERIA,
    ARABIC_BAHRAIN,
    ARABIC_EGYPT,
    ARABIC_IRAQ,
    ARABIC_JORDAN,
    ARABIC_KUWAIT,
    ARABIC_LEBANON,
    ARABIC_LIBYA,
    ARABIC_MOROCCO,
    ARABIC_OMAN,
    ARABIC_QATAR,
    ARABIC_SAUDI_ARABIA,
    ARABIC_SUDAN,
    ARABIC_SYRIA,
    ARABIC_TUNISIA,
    ARABIC_UAE,
    ARABIC_YEMEN,
    ARMENIAN,
    ASSAMESE,
    AYMARA,
    AZERI,
    AZERI_CYRILLIC,
    AZERI_LATIN,
    BASHKIR,
    BASQUE,
    BELARUSIAN,
    BENGALI,
    BHUTANI,
    BIHARI,
    BISLAMA,
    BRETON,
    BULGARIAN,
    BURMESE,
    CAMBODIAN,
    CATALAN,
    CHINESE,
    CHINESE_SIMPLIFIED,
    CHINESE_TRADITIONAL,
    CHINESE_HONGKONG,
    CHINESE_MACAU,
    CHINESE_SINGAPORE,
    CHINESE_TAIWAN,
    CORSICAN,
    CROATIAN,
    CZECH,
    DANISH,
    DUTCH,
    DUTCH_BELGIAN,
    ENGLISH,
    ENGLISH_UK,
    ENGLISH_US,
    ENGLISH_AUSTRALIA,
    ENGLISH_BELIZE,
    ENGLISH_BOTSWANA,
    ENGLISH_CANADA,
    ENGLISH_CARIBBEAN,
    ENGLISH_DENMARK,
    ENGLISH_EIRE,
    ENGLISH_JAMAICA,
    ENGLISH_NEW_ZEALAND,
    ENGLISH_PHILIPPINES,
    ENGLISH_SOUTH_AFRICA,
    ENGLISH_TRINIDAD,
    ENGLISH_ZIMBABWE,
    ESPERANTO,
    ESTONIAN,
    FAEROESE,
    FARSI,
    FIJI,
    FINNISH,
    FRENCH,
    FRENCH_BELGIAN,
    FRENCH_CANADIAN,
    FRENCH_LUXEMBOURG,
    FRENCH_MONACO,
    FRENCH_SWISS,
    FRISIAN,
    GALICIAN,
    GEORGIAN,
    GERMAN,
    GERMAN_AUSTRIAN,
    GERMAN_BELGIUM,
    GERMAN_LIECHTENSTEIN,
    GERMAN_LUXEMBOURG,
    GERMAN_SWISS,
    GREEK,
    GREENLANDIC,
    GUARANI,
    GUJARATI,
    HAUSA,
    HEBREW,
    HINDI,
    HUNGARIAN,
    ICELANDIC,
    INDONESIAN,
    INTERLINGUA,
    INTERLINGUE,
    INUKTITUT,
    INUPIAK,
    IRISH,
    ITALIAN,
    ITALIAN_SWISS,
    JAPANESE,
    JAVANESE,
    KANNADA,
    KASHMIRI,
    KASHMIRI_INDIA,
    KAZAKH,
    KERNEWEK,
    KINYARWANDA,
    KIRGHIZ,
    KIRUNDI,
    KONKANI,
    KOREAN,
    KURDISH,
    LAOTHIAN,
    LATIN,
    LATVIAN,
    LINGALA,
    LITHUANIAN,
    MACEDONIAN,
    MALAGASY,
    MALAY,
    MALAYALAM,
    MALAY_BRUNEI_DARUSSALAM,
    MALAY_MALAYSIA,
    MALTESE,
    MANIPURI,
    MAORI,
    MARATHI,
    MOLDAVIAN,
    MONGOLIAN,
    NAURU,
    NEPALI,
    NEPALI_INDIA,
    NORWEGIAN_BOKMAL,
    NORWEGIAN_NYNORSK,
    OCCITAN,
    ORIYA,
    OROMO,
    PASHTO,
    POLISH,
    PORTUGUESE,
    PORTUGUESE_BRAZILIAN,
    PUNJABI,
    QUECHUA,
    RHAETO_ROMANCE,
    ROMANIAN,
    RUSSIAN,
    RUSSIAN_UKRAINE,
    SAMOAN,
    SANGHO,
    SANSKRIT,
    SCOTS_GAELIC,
    SERBIAN,
    SERBIAN_CYRILLIC,
    SERBIAN_LATIN,
    SERBO_CROATIAN,
    SESOTHO,
    SETSWANA,
    SHONA,
    SINDHI,
    SINHALESE,
    SISWATI,
    SLOVAK,
    SLOVENIAN,
    SOMALI,
    SPANISH,
    SPANISH_ARGENTINA,
    SPANISH_BOLIVIA,
    SPANISH_CHILE,
    SPANISH_COLOMBIA,
    SPANISH_COSTA_RICA,
    SPANISH_DOMINICAN_REPUBLIC,
    SPANISH_ECUADOR,
    SPANISH_EL_SALVADOR,
    SPANISH_GUATEMALA,
    SPANISH_HONDURAS,
    SPANISH_MEXICAN,
    SPANISH_MODERN,
    SPANISH_NICARAGUA,
    SPANISH_PANAMA,
    SPANISH_PARAGUAY,
    SPANISH_PERU,
    SPANISH_PUERTO_RICO,
    SPANISH_URUGUAY,
    SPANISH_US,
    SPANISH_VENEZUELA,
    SUNDANESE,
    SWAHILI,
    SWEDISH,
    SWEDISH_FINLAND,
    TAGALOG,
    TAJIK,
    TAMIL,
    TATAR,
    TELUGU,
    THAI,
    TIBETAN,
    TIGRINYA,
    TONGA,
    TSONGA,
    TURKISH,
    TURKMEN,
    TWI,
    UIGHUR,
    UKRAINIAN,
    URDU,
    URDU_INDIA,
    URDU_PAKISTAN,
    UZBEK,
    UZBEK_CYRILLIC,
    UZBEK_LATIN,
    VIETNAMESE,
    VOLAPUK,
    WELSH,
    WOLOF,
    XHOSA,
    YIDDISH,
    YORUBA,
    ZHUANG,
    ZULU,
    UNKNOWN // should be the last one
  };
  //ETX

  // Description:
  // Set the current language. This is done by setting the LC_MESSAGES locale
  // as well as setting the LC_MESSAGES environment variable. On Windows
  // platform where LC_MESSAGES is not supported, a call to SetThreadLocale()
  // will change the language accordingly.
  static void SetCurrentLanguage(int lang);

  // Description:
  // Get short English name of language (or NULL if unknown/error).
  static const char* GetLanguageName(int lang);

  // Description:
  // Get XPG syntax (language[_territory[.codeset]][@modifier]) from language.
  // Return XPG description on success, NULL on error or if there is
  // no known XPG syntax for this language ID.
  static const char* GetXPGFromLanguage(int lang);

  // Description:
  // Get language from XPG (language[_territory[.codeset]][@modifier]).
  // Return language ID on success, vtkKWLanguage::UNKNOWN on error or if 
  // there is no known language ID for this XPG syntax.
  static int GetLanguageFromXPG(const char *xpg);

  // Description:
  // Get Win32 LANGID from language.
  // Return the output of MAKELANGID using the primary and secondary
  // language identifier corresponding to the language passed as parameter,
  // or MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) if no match was found.
  // (note that MAKELANGID returns a WORD, which is cast here to an int
  // for wrapping purposes).
  static int GetWin32LANGIDFromLanguage(int lang);

  // Description:
  // Get language from Win32 LANGID.
  // Return the language id matching the Win32 primary and secondary language
  // identifier that were output by MAKELANGID, or vtkKWLanguage::UNKNOWN on
  // error or if there is no known language ID for this LANGID.
  // (note that MAKELANGID returns a WORD, but it is accepted here as an int
  // for wrapping purposes).
  static int GetLanguageFromWin32LANGID(int win32langid);

protected:
  vtkKWLanguage() {};
  ~vtkKWLanguage() {};

  // Description:
  // Get language from XPG (language[_territory[.codeset]][@modifier]).
  // Return language ID on success, vtkKWLanguage::UNKNOWN on error or if 
  // there is no known language ID for this XPG syntax.
  // This is a stricter version GetLanguageFromXPG since it does not try
  // to find the language if the territory was missing.
  static int GetLanguageFromXPGStrict(const char *xpg);


private:
  vtkKWLanguage(const vtkKWLanguage&); // Not implemented
  void operator=(const vtkKWLanguage&); // Not implemented
};

#endif

