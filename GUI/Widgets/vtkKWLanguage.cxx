/*=========================================================================

  Module:    vtkKWLanguage.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWLanguage.h"

#ifdef _WIN32
#include "windows.h"
#else
#include <locale.h>
#endif
#include "vtkKWInternationalization.h"

#include "vtkObjectFactory.h"
#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLanguage);
vtkCxxRevisionMacro(vtkKWLanguage, "1.5");

//----------------------------------------------------------------------------
void vtkKWLanguage::SetCurrentLanguage(int lang)
{
  const char *xpg = vtkKWLanguage::GetXPGFromLanguage(lang);
  if (xpg)
    {
#ifndef _WIN32
    setlocale(LC_MESSAGES, xpg);
#endif

    // Setting this environment variable can help libraries like
    // GNU's gettext, but does not work either on Win32 since the
    // gettext DLL has its own environment.
    // gettext will also try LC_ALL, LC_xxx (i.e. LC_MESSAGES), and LANG

    vtksys_stl::string env_var;

    env_var = "LC_MESSAGES=";
    env_var += xpg;
    putenv((char*)env_var.c_str());

    env_var = "LANG=";
    env_var += xpg;
    putenv((char*)env_var.c_str());
    
    /* According to gettext doc, section 10.5
       The code for gcc-2.7.0 and up provides some optimization. This 
       optimization normally prevents the calling of the dcgettext function as
       long as no new catalog is loaded. But if dcgettext is not called the
       program also cannot find the LANGUAGE variable has changed.
       Here is the "recommended" solution
    */
#if !defined(_WIN32) && defined(KWWidgets_USE_INTERNATIONALIZATION)
    {
      extern int  _nl_msg_cat_cntr;
      ++_nl_msg_cat_cntr;
    }
#endif
    }

#ifdef _WIN32
  ::SetThreadLocale(
    MAKELCID(vtkKWLanguage::GetWin32LANGIDFromLanguage(lang), SORT_DEFAULT));
#endif  
}

//----------------------------------------------------------------------------
int vtkKWLanguage::GetCurrentLanguage()
{
#ifndef _WIN32
  return vtkKWLanguage::GetLanguageFromXPG(setlocale(LC_MESSAGES, "NULL"));
#else
  return vtkKWLanguage::GetLanguageFromWin32LANGID(
    LANGIDFROMLCID(::GetThreadLocale()));
#endif  
}

//----------------------------------------------------------------------------
const char* vtkKWLanguage::GetLanguageName(int lang)
{
  switch (lang)
    {
    case vtkKWLanguage::ABKHAZIAN:
      return "Abkhazian";
    case vtkKWLanguage::AFAR:
      return "Afar";
    case vtkKWLanguage::AFRIKAANS:
      return "Afrikaans";
    case vtkKWLanguage::ALBANIAN:
      return "Albanian";
    case vtkKWLanguage::AMHARIC:
      return "Amharic";
    case vtkKWLanguage::ARABIC:
      return "Arabic";
    case vtkKWLanguage::ARABIC_ALGERIA:
      return "Arabic (Algeria)";
    case vtkKWLanguage::ARABIC_BAHRAIN:
      return "Arabic (Bahrain)";
    case vtkKWLanguage::ARABIC_EGYPT:
      return "Arabic (Egypt)";
    case vtkKWLanguage::ARABIC_IRAQ:
      return "Arabic (Iraq)";
    case vtkKWLanguage::ARABIC_JORDAN:
      return "Arabic (Jordan)";
    case vtkKWLanguage::ARABIC_KUWAIT:
      return "Arabic (Kuwait)";
    case vtkKWLanguage::ARABIC_LEBANON:
      return "Arabic (Lebanon)";
    case vtkKWLanguage::ARABIC_LIBYA:
      return "Arabic (Libya)";
    case vtkKWLanguage::ARABIC_MOROCCO:
      return "Arabic (Morocco)";
    case vtkKWLanguage::ARABIC_OMAN:
      return "Arabic (Oman)";
    case vtkKWLanguage::ARABIC_QATAR:
      return "Arabic (Qatar)";
    case vtkKWLanguage::ARABIC_SAUDI_ARABIA:
      return "Arabic (Saudi Arabia)";
    case vtkKWLanguage::ARABIC_SUDAN:
      return "Arabic (Sudan)";
    case vtkKWLanguage::ARABIC_SYRIA:
      return "Arabic (Syria)";
    case vtkKWLanguage::ARABIC_TUNISIA:
      return "Arabic (Tunisia)";
    case vtkKWLanguage::ARABIC_UAE:
      return "Arabic (Uae)";
    case vtkKWLanguage::ARABIC_YEMEN:
      return "Arabic (Yemen)";
    case vtkKWLanguage::ARMENIAN:
      return "Armenian";
    case vtkKWLanguage::ASSAMESE:
      return "Assamese";
    case vtkKWLanguage::AYMARA:
      return "Aymara";
    case vtkKWLanguage::AZERI:
      return "Azeri";
    case vtkKWLanguage::AZERI_CYRILLIC:
      return "Azeri (Cyrillic)";
    case vtkKWLanguage::AZERI_LATIN:
      return "Azeri (Latin)";
    case vtkKWLanguage::BASHKIR:
      return "Bashkir";
    case vtkKWLanguage::BASQUE:
      return "Basque";
    case vtkKWLanguage::BELARUSIAN:
      return "Belarusian";
    case vtkKWLanguage::BENGALI:
      return "Bengali";
    case vtkKWLanguage::BHUTANI:
      return "Bhutani";
    case vtkKWLanguage::BIHARI:
      return "Bihari";
    case vtkKWLanguage::BISLAMA:
      return "Bislama";
    case vtkKWLanguage::BRETON:
      return "Breton";
    case vtkKWLanguage::BULGARIAN:
      return "Bulgarian";
    case vtkKWLanguage::BURMESE:
      return "Burmese";
    case vtkKWLanguage::CAMBODIAN:
      return "Cambodian";
    case vtkKWLanguage::CATALAN:
      return "Catalan";
    case vtkKWLanguage::CHINESE:
      return "Chinese";
    case vtkKWLanguage::CHINESE_SIMPLIFIED:
      return "Chinese (Simplified)";
    case vtkKWLanguage::CHINESE_TRADITIONAL:
      return "Chinese (Traditional)";
    case vtkKWLanguage::CHINESE_HONGKONG:
      return "Chinese (Hongkong)";
    case vtkKWLanguage::CHINESE_MACAU:
      return "Chinese (Macau)";
    case vtkKWLanguage::CHINESE_SINGAPORE:
      return "Chinese (Singapore)";
    case vtkKWLanguage::CHINESE_TAIWAN:
      return "Chinese (Taiwan)";
    case vtkKWLanguage::CORSICAN:
      return "Corsican";
    case vtkKWLanguage::CROATIAN:
      return "Croatian";
    case vtkKWLanguage::CZECH:
      return "Czech";
    case vtkKWLanguage::DANISH:
      return "Danish";
    case vtkKWLanguage::DUTCH:
      return "Dutch";
    case vtkKWLanguage::DUTCH_BELGIAN:
      return "Dutch (Belgian)";
    case vtkKWLanguage::ENGLISH:
      return "English";
    case vtkKWLanguage::ENGLISH_UK:
      return "English (U.K.)";
    case vtkKWLanguage::ENGLISH_US:
      return "English (U.S.)";
    case vtkKWLanguage::ENGLISH_AUSTRALIA:
      return "English (Australia)";
    case vtkKWLanguage::ENGLISH_BELIZE:
      return "English (Belize)";
    case vtkKWLanguage::ENGLISH_BOTSWANA:
      return "English (Botswana)";
    case vtkKWLanguage::ENGLISH_CANADA:
      return "English (Canada)";
    case vtkKWLanguage::ENGLISH_CARIBBEAN:
      return "English (Caribbean)";
    case vtkKWLanguage::ENGLISH_DENMARK:
      return "English (Denmark)";
    case vtkKWLanguage::ENGLISH_EIRE:
      return "English (Eire)";
    case vtkKWLanguage::ENGLISH_JAMAICA:
      return "English (Jamaica)";
    case vtkKWLanguage::ENGLISH_NEW_ZEALAND:
      return "English (New Zealand)";
    case vtkKWLanguage::ENGLISH_PHILIPPINES:
      return "English (Philippines)";
    case vtkKWLanguage::ENGLISH_SOUTH_AFRICA:
      return "English (South Africa)";
    case vtkKWLanguage::ENGLISH_TRINIDAD:
      return "English (Trinidad)";
    case vtkKWLanguage::ENGLISH_ZIMBABWE:
      return "English (Zimbabwe)";
    case vtkKWLanguage::ESPERANTO:
      return "Esperanto";
    case vtkKWLanguage::ESTONIAN:
      return "Estonian";
    case vtkKWLanguage::FAEROESE:
      return "Faeroese";
    case vtkKWLanguage::FARSI:
      return "Farsi";
    case vtkKWLanguage::FIJI:
      return "Fiji";
    case vtkKWLanguage::FINNISH:
      return "Finnish";
    case vtkKWLanguage::FRENCH:
      return "French";
    case vtkKWLanguage::FRENCH_BELGIAN:
      return "French (Belgian)";
    case vtkKWLanguage::FRENCH_CANADIAN:
      return "French (Canadian)";
    case vtkKWLanguage::FRENCH_LUXEMBOURG:
      return "French (Luxembourg)";
    case vtkKWLanguage::FRENCH_MONACO:
      return "French (Monaco)";
    case vtkKWLanguage::FRENCH_SWISS:
      return "French (Swiss)";
    case vtkKWLanguage::FRISIAN:
      return "Frisian";
    case vtkKWLanguage::GALICIAN:
      return "Galician";
    case vtkKWLanguage::GEORGIAN:
      return "Georgian";
    case vtkKWLanguage::GERMAN:
      return "German";
    case vtkKWLanguage::GERMAN_AUSTRIAN:
      return "German (Austrian)";
    case vtkKWLanguage::GERMAN_BELGIUM:
      return "German (Belgium)";
    case vtkKWLanguage::GERMAN_LIECHTENSTEIN:
      return "German (Liechtenstein)";
    case vtkKWLanguage::GERMAN_LUXEMBOURG:
      return "German (Luxembourg)";
    case vtkKWLanguage::GERMAN_SWISS:
      return "German (Swiss)";
    case vtkKWLanguage::GREEK:
      return "Greek";
    case vtkKWLanguage::GREENLANDIC:
      return "Greenlandic";
    case vtkKWLanguage::GUARANI:
      return "Guarani";
    case vtkKWLanguage::GUJARATI:
      return "Gujarati";
    case vtkKWLanguage::HAUSA:
      return "Hausa";
    case vtkKWLanguage::HEBREW:
      return "Hebrew";
    case vtkKWLanguage::HINDI:
      return "Hindi";
    case vtkKWLanguage::HUNGARIAN:
      return "Hungarian";
    case vtkKWLanguage::ICELANDIC:
      return "Icelandic";
    case vtkKWLanguage::INDONESIAN:
      return "Indonesian";
    case vtkKWLanguage::INTERLINGUA:
      return "Interlingua";
    case vtkKWLanguage::INTERLINGUE:
      return "Interlingue";
    case vtkKWLanguage::INUKTITUT:
      return "Inuktitut";
    case vtkKWLanguage::INUPIAK:
      return "Inupiak";
    case vtkKWLanguage::IRISH:
      return "Irish";
    case vtkKWLanguage::ITALIAN:
      return "Italian";
    case vtkKWLanguage::ITALIAN_SWISS:
      return "Italian (Swiss)";
    case vtkKWLanguage::JAPANESE:
      return "Japanese";
    case vtkKWLanguage::JAVANESE:
      return "Javanese";
    case vtkKWLanguage::KANNADA:
      return "Kannada";
    case vtkKWLanguage::KASHMIRI:
      return "Kashmiri";
    case vtkKWLanguage::KASHMIRI_INDIA:
      return "Kashmiri (India)";
    case vtkKWLanguage::KAZAKH:
      return "Kazakh";
    case vtkKWLanguage::KERNEWEK:
      return "Kernewek";
    case vtkKWLanguage::KINYARWANDA:
      return "Kinyarwanda";
    case vtkKWLanguage::KIRGHIZ:
      return "Kirghiz";
    case vtkKWLanguage::KIRUNDI:
      return "Kirundi";
    case vtkKWLanguage::KONKANI:
      return "Konkani";
    case vtkKWLanguage::KOREAN:
      return "Korean";
    case vtkKWLanguage::KURDISH:
      return "Kurdish";
    case vtkKWLanguage::LAOTHIAN:
      return "Laothian";
    case vtkKWLanguage::LATIN:
      return "Latin";
    case vtkKWLanguage::LATVIAN:
      return "Latvian";
    case vtkKWLanguage::LINGALA:
      return "Lingala";
    case vtkKWLanguage::LITHUANIAN:
      return "Lithuanian";
    case vtkKWLanguage::MACEDONIAN:
      return "Macedonian";
    case vtkKWLanguage::MALAGASY:
      return "Malagasy";
    case vtkKWLanguage::MALAY:
      return "Malay";
    case vtkKWLanguage::MALAYALAM:
      return "Malayalam";
    case vtkKWLanguage::MALAY_BRUNEI_DARUSSALAM:
      return "Malay (Brunei Darussalam)";
    case vtkKWLanguage::MALAY_MALAYSIA:
      return "Malay (Malaysia)";
    case vtkKWLanguage::MALTESE:
      return "Maltese";
    case vtkKWLanguage::MANIPURI:
      return "Manipuri";
    case vtkKWLanguage::MAORI:
      return "Maori";
    case vtkKWLanguage::MARATHI:
      return "Marathi";
    case vtkKWLanguage::MOLDAVIAN:
      return "Moldavian";
    case vtkKWLanguage::MONGOLIAN:
      return "Mongolian";
    case vtkKWLanguage::NAURU:
      return "Nauru";
    case vtkKWLanguage::NEPALI:
      return "Nepali";
    case vtkKWLanguage::NEPALI_INDIA:
      return "Nepali (India)";
    case vtkKWLanguage::NORWEGIAN_BOKMAL:
      return "Norwegian (Bokmal)";
    case vtkKWLanguage::NORWEGIAN_NYNORSK:
      return "Norwegian (Nynorsk)";
    case vtkKWLanguage::OCCITAN:
      return "Occitan";
    case vtkKWLanguage::ORIYA:
      return "Oriya";
    case vtkKWLanguage::OROMO:
      return "(Afan) Oromo";
    case vtkKWLanguage::PASHTO:
      return "Pashto, Pushto";
    case vtkKWLanguage::POLISH:
      return "Polish";
    case vtkKWLanguage::PORTUGUESE:
      return "Portuguese";
    case vtkKWLanguage::PORTUGUESE_BRAZILIAN:
      return "Portuguese (Brazilian)";
    case vtkKWLanguage::PUNJABI:
      return "Punjabi";
    case vtkKWLanguage::QUECHUA:
      return "Quechua";
    case vtkKWLanguage::RHAETO_ROMANCE:
      return "Rhaeto-Romance";
    case vtkKWLanguage::ROMANIAN:
      return "Romanian";
    case vtkKWLanguage::RUSSIAN:
      return "Russian";
    case vtkKWLanguage::RUSSIAN_UKRAINE:
      return "Russian (Ukraine)";
    case vtkKWLanguage::SAMOAN:
      return "Samoan";
    case vtkKWLanguage::SANGHO:
      return "Sangho";
    case vtkKWLanguage::SANSKRIT:
      return "Sanskrit";
    case vtkKWLanguage::SCOTS_GAELIC:
      return "Scots Gaelic";
    case vtkKWLanguage::SERBIAN:
      return "Serbian";
    case vtkKWLanguage::SERBIAN_CYRILLIC:
      return "Serbian (Cyrillic)";
    case vtkKWLanguage::SERBIAN_LATIN:
      return "Serbian (Latin)";
    case vtkKWLanguage::SERBO_CROATIAN:
      return "Serbo-Croatian";
    case vtkKWLanguage::SESOTHO:
      return "Sesotho";
    case vtkKWLanguage::SETSWANA:
      return "Setswana";
    case vtkKWLanguage::SHONA:
      return "Shona";
    case vtkKWLanguage::SINDHI:
      return "Sindhi";
    case vtkKWLanguage::SINHALESE:
      return "Sinhalese";
    case vtkKWLanguage::SISWATI:
      return "Siswati";
    case vtkKWLanguage::SLOVAK:
      return "Slovak";
    case vtkKWLanguage::SLOVENIAN:
      return "Slovenian";
    case vtkKWLanguage::SOMALI:
      return "Somali";
    case vtkKWLanguage::SPANISH:
      return "Spanish";
    case vtkKWLanguage::SPANISH_ARGENTINA:
      return "Spanish (Argentina)";
    case vtkKWLanguage::SPANISH_BOLIVIA:
      return "Spanish (Bolivia)";
    case vtkKWLanguage::SPANISH_CHILE:
      return "Spanish (Chile)";
    case vtkKWLanguage::SPANISH_COLOMBIA:
      return "Spanish (Colombia)";
    case vtkKWLanguage::SPANISH_COSTA_RICA:
      return "Spanish (Costa Rica)";
    case vtkKWLanguage::SPANISH_DOMINICAN_REPUBLIC:
      return "Spanish (Dominican republic)";
    case vtkKWLanguage::SPANISH_ECUADOR:
      return "Spanish (Ecuador)";
    case vtkKWLanguage::SPANISH_EL_SALVADOR:
      return "Spanish (El Salvador)";
    case vtkKWLanguage::SPANISH_GUATEMALA:
      return "Spanish (Guatemala)";
    case vtkKWLanguage::SPANISH_HONDURAS:
      return "Spanish (Honduras)";
    case vtkKWLanguage::SPANISH_MEXICAN:
      return "Spanish (Mexican)";
    case vtkKWLanguage::SPANISH_MODERN:
      return "Spanish (Modern)";
    case vtkKWLanguage::SPANISH_NICARAGUA:
      return "Spanish (Nicaragua)";
    case vtkKWLanguage::SPANISH_PANAMA:
      return "Spanish (Panama)";
    case vtkKWLanguage::SPANISH_PARAGUAY:
      return "Spanish (Paraguay)";
    case vtkKWLanguage::SPANISH_PERU:
      return "Spanish (Peru)";
    case vtkKWLanguage::SPANISH_PUERTO_RICO:
      return "Spanish (Puerto Rico)";
    case vtkKWLanguage::SPANISH_URUGUAY:
      return "Spanish (Uruguay)";
    case vtkKWLanguage::SPANISH_US:
      return "Spanish (U.S.)";
    case vtkKWLanguage::SPANISH_VENEZUELA:
      return "Spanish (Venezuela)";
    case vtkKWLanguage::SUNDANESE:
      return "Sundanese";
    case vtkKWLanguage::SWAHILI:
      return "Swahili";
    case vtkKWLanguage::SWEDISH:
      return "Swedish";
    case vtkKWLanguage::SWEDISH_FINLAND:
      return "Swedish (Finland)";
    case vtkKWLanguage::TAGALOG:
      return "Tagalog";
    case vtkKWLanguage::TAJIK:
      return "Tajik";
    case vtkKWLanguage::TAMIL:
      return "Tamil";
    case vtkKWLanguage::TATAR:
      return "Tatar";
    case vtkKWLanguage::TELUGU:
      return "Telugu";
    case vtkKWLanguage::THAI:
      return "Thai";
    case vtkKWLanguage::TIBETAN:
      return "Tibetan";
    case vtkKWLanguage::TIGRINYA:
      return "Tigrinya";
    case vtkKWLanguage::TONGA:
      return "Tonga";
    case vtkKWLanguage::TSONGA:
      return "Tsonga";
    case vtkKWLanguage::TURKISH:
      return "Turkish";
    case vtkKWLanguage::TURKMEN:
      return "Turkmen";
    case vtkKWLanguage::TWI:
      return "Twi";
    case vtkKWLanguage::UIGHUR:
      return "Uighur";
    case vtkKWLanguage::UKRAINIAN:
      return "Ukrainian";
    case vtkKWLanguage::URDU:
      return "Urdu";
    case vtkKWLanguage::URDU_INDIA:
      return "Urdu (India)";
    case vtkKWLanguage::URDU_PAKISTAN:
      return "Urdu (Pakistan)";
    case vtkKWLanguage::UZBEK:
      return "Uzbek";
    case vtkKWLanguage::UZBEK_CYRILLIC:
      return "Uzbek (Cyrillic)";
    case vtkKWLanguage::UZBEK_LATIN:
      return "Uzbek (Latin)";
    case vtkKWLanguage::VIETNAMESE:
      return "Vietnamese";
    case vtkKWLanguage::VOLAPUK:
      return "Volapuk";
    case vtkKWLanguage::WELSH:
      return "Welsh";
    case vtkKWLanguage::WOLOF:
      return "Wolof";
    case vtkKWLanguage::XHOSA:
      return "Xhosa";
    case vtkKWLanguage::YIDDISH:
      return "Yiddish";
    case vtkKWLanguage::YORUBA:
      return "Yoruba";
    case vtkKWLanguage::ZHUANG:
      return "Zhuang";
    case vtkKWLanguage::ZULU:
      return "Zulu";

    default: 
      return NULL;
    }
}

//----------------------------------------------------------------------------
const char* vtkKWLanguage::GetXPGFromLanguage(int lang)
{
  switch (lang)
    {
    case vtkKWLanguage::ABKHAZIAN:
      return "ab";
    case vtkKWLanguage::AFAR:
      return "aa";
    case vtkKWLanguage::AFRIKAANS:
      return "af_ZA";
    case vtkKWLanguage::ALBANIAN:
      return "sq_AL";
    case vtkKWLanguage::AMHARIC:
      return "am";
    case vtkKWLanguage::ARABIC:
      return "ar";
    case vtkKWLanguage::ARABIC_ALGERIA:
      return "ar_DZ";
    case vtkKWLanguage::ARABIC_BAHRAIN:
      return "ar_BH";
    case vtkKWLanguage::ARABIC_EGYPT:
      return "ar_EG";
    case vtkKWLanguage::ARABIC_IRAQ:
      return "ar_IQ";
    case vtkKWLanguage::ARABIC_JORDAN:
      return "ar_JO";
    case vtkKWLanguage::ARABIC_KUWAIT:
      return "ar_KW";
    case vtkKWLanguage::ARABIC_LEBANON:
      return "ar_LB";
    case vtkKWLanguage::ARABIC_LIBYA:
      return "ar_LY";
    case vtkKWLanguage::ARABIC_MOROCCO:
      return "ar_MA";
    case vtkKWLanguage::ARABIC_OMAN:
      return "ar_OM";
    case vtkKWLanguage::ARABIC_QATAR:
      return "ar_QA";
    case vtkKWLanguage::ARABIC_SAUDI_ARABIA:
      return "ar_SA";
    case vtkKWLanguage::ARABIC_SUDAN:
      return "ar_SD";
    case vtkKWLanguage::ARABIC_SYRIA:
      return "ar_SY";
    case vtkKWLanguage::ARABIC_TUNISIA:
      return "ar_TN";
    case vtkKWLanguage::ARABIC_UAE:
      return "ar_AE";
    case vtkKWLanguage::ARABIC_YEMEN:
      return "ar_YE";
    case vtkKWLanguage::ARMENIAN:
      return "hy";
    case vtkKWLanguage::ASSAMESE:
      return "as";
    case vtkKWLanguage::AYMARA:
      return "ay";
    case vtkKWLanguage::AZERI:
      return "az";
    case vtkKWLanguage::AZERI_CYRILLIC:
      return "az";
    case vtkKWLanguage::AZERI_LATIN:
      return "az";
    case vtkKWLanguage::BASHKIR:
      return "ba";
    case vtkKWLanguage::BASQUE:
      return "eu_ES";
    case vtkKWLanguage::BELARUSIAN:
      return "be_BY";
    case vtkKWLanguage::BENGALI:
      return "bn";
    case vtkKWLanguage::BHUTANI:
      return "dz";
    case vtkKWLanguage::BIHARI:
      return "bh";
    case vtkKWLanguage::BISLAMA:
      return "bi";
    case vtkKWLanguage::BRETON:
      return "br";
    case vtkKWLanguage::BULGARIAN:
      return "bg_BG";
    case vtkKWLanguage::BURMESE:
      return "my";
    case vtkKWLanguage::CAMBODIAN:
      return "km";
    case vtkKWLanguage::CATALAN:
      return "ca_ES";
    case vtkKWLanguage::CHINESE:
      return "zh_TW";
    case vtkKWLanguage::CHINESE_SIMPLIFIED:
      return "zh_CN";
    case vtkKWLanguage::CHINESE_TRADITIONAL:
      return "zh_TW";
    case vtkKWLanguage::CHINESE_HONGKONG:
      return "zh_HK";
    case vtkKWLanguage::CHINESE_MACAU:
      return "zh_MO";
    case vtkKWLanguage::CHINESE_SINGAPORE:
      return "zh_SG";
    case vtkKWLanguage::CHINESE_TAIWAN:
      return "zh_TW";
    case vtkKWLanguage::CORSICAN:
      return "co";
    case vtkKWLanguage::CROATIAN:
      return "hr_HR";
    case vtkKWLanguage::CZECH:
      return "cs_CZ";
    case vtkKWLanguage::DANISH:
      return "da_DK";
    case vtkKWLanguage::DUTCH:
      return "nl_NL";
    case vtkKWLanguage::DUTCH_BELGIAN:
      return "nl_BE";
    case vtkKWLanguage::ENGLISH:
      return "en_GB";
    case vtkKWLanguage::ENGLISH_UK:
      return "en_GB";
    case vtkKWLanguage::ENGLISH_US:
      return "en_US";
    case vtkKWLanguage::ENGLISH_AUSTRALIA:
      return "en_AU";
    case vtkKWLanguage::ENGLISH_BELIZE:
      return "en_BZ";
    case vtkKWLanguage::ENGLISH_BOTSWANA:
      return "en_BW";
    case vtkKWLanguage::ENGLISH_CANADA:
      return "en_CA";
    case vtkKWLanguage::ENGLISH_CARIBBEAN:
      return "en_CB";
    case vtkKWLanguage::ENGLISH_DENMARK:
      return "en_DK";
    case vtkKWLanguage::ENGLISH_EIRE:
      return "en_IE";
    case vtkKWLanguage::ENGLISH_JAMAICA:
      return "en_JM";
    case vtkKWLanguage::ENGLISH_NEW_ZEALAND:
      return "en_NZ";
    case vtkKWLanguage::ENGLISH_PHILIPPINES:
      return "en_PH";
    case vtkKWLanguage::ENGLISH_SOUTH_AFRICA:
      return "en_ZA";
    case vtkKWLanguage::ENGLISH_TRINIDAD:
      return "en_TT";
    case vtkKWLanguage::ENGLISH_ZIMBABWE:
      return "en_ZW";
    case vtkKWLanguage::ESPERANTO:
      return "eo";
    case vtkKWLanguage::ESTONIAN:
      return "et_EE";
    case vtkKWLanguage::FAEROESE:
      return "fo_FO";
    case vtkKWLanguage::FARSI:
      return "fa_IR";
    case vtkKWLanguage::FIJI:
      return "fj";
    case vtkKWLanguage::FINNISH:
      return "fi_FI";
    case vtkKWLanguage::FRENCH:
      return "fr_FR";
    case vtkKWLanguage::FRENCH_BELGIAN:
      return "fr_BE";
    case vtkKWLanguage::FRENCH_CANADIAN:
      return "fr_CA";
    case vtkKWLanguage::FRENCH_LUXEMBOURG:
      return "fr_LU";
    case vtkKWLanguage::FRENCH_MONACO:
      return "fr_MC";
    case vtkKWLanguage::FRENCH_SWISS:
      return "fr_CH";
    case vtkKWLanguage::FRISIAN:
      return "fy";
    case vtkKWLanguage::GALICIAN:
      return "gl_ES";
    case vtkKWLanguage::GEORGIAN:
      return "ka";
    case vtkKWLanguage::GERMAN:
      return "de_DE";
    case vtkKWLanguage::GERMAN_AUSTRIAN:
      return "de_AT";
    case vtkKWLanguage::GERMAN_BELGIUM:
      return "de_BE";
    case vtkKWLanguage::GERMAN_LIECHTENSTEIN:
      return "de_LI";
    case vtkKWLanguage::GERMAN_LUXEMBOURG:
      return "de_LU";
    case vtkKWLanguage::GERMAN_SWISS:
      return "de_CH";
    case vtkKWLanguage::GREEK:
      return "el_GR";
    case vtkKWLanguage::GREENLANDIC:
      return "kl_GL";
    case vtkKWLanguage::GUARANI:
      return "gn";
    case vtkKWLanguage::GUJARATI:
      return "gu";
    case vtkKWLanguage::HAUSA:
      return "ha";
    case vtkKWLanguage::HEBREW:
      return "he_IL";
    case vtkKWLanguage::HINDI:
      return "hi_IN";
    case vtkKWLanguage::HUNGARIAN:
      return "hu_HU";
    case vtkKWLanguage::ICELANDIC:
      return "is_IS";
    case vtkKWLanguage::INDONESIAN:
      return "id_ID";
    case vtkKWLanguage::INTERLINGUA:
      return "ia";
    case vtkKWLanguage::INTERLINGUE:
      return "ie";
    case vtkKWLanguage::INUKTITUT:
      return "iu";
    case vtkKWLanguage::INUPIAK:
      return "ik";
    case vtkKWLanguage::IRISH:
      return "ga_IE";
    case vtkKWLanguage::ITALIAN:
      return "it_IT";
    case vtkKWLanguage::ITALIAN_SWISS:
      return "it_CH";
    case vtkKWLanguage::JAPANESE:
      return "ja_JP";
    case vtkKWLanguage::JAVANESE:
      return "jw";
    case vtkKWLanguage::KANNADA:
      return "kn";
    case vtkKWLanguage::KASHMIRI:
      return "ks";
    case vtkKWLanguage::KASHMIRI_INDIA:
      return "ks_IN";
    case vtkKWLanguage::KAZAKH:
      return "kk";
    case vtkKWLanguage::KERNEWEK:
      return "kw_GB";
    case vtkKWLanguage::KINYARWANDA:
      return "rw";
    case vtkKWLanguage::KIRGHIZ:
      return "ky";
    case vtkKWLanguage::KIRUNDI:
      return "rn";
    case vtkKWLanguage::KOREAN:
      return "ko_KR";
    case vtkKWLanguage::KURDISH:
      return "ku";
    case vtkKWLanguage::LAOTHIAN:
      return "lo";
    case vtkKWLanguage::LATIN:
      return "la";
    case vtkKWLanguage::LATVIAN:
      return "lv_LV";
    case vtkKWLanguage::LINGALA:
      return "ln";
    case vtkKWLanguage::LITHUANIAN:
      return "lt_LT";
    case vtkKWLanguage::MACEDONIAN:
      return "mk_MK";
    case vtkKWLanguage::MALAGASY:
      return "mg";
    case vtkKWLanguage::MALAY:
      return "ms_MY";
    case vtkKWLanguage::MALAYALAM:
      return "ml";
    case vtkKWLanguage::MALAY_BRUNEI_DARUSSALAM:
      return "ms_BN";
    case vtkKWLanguage::MALAY_MALAYSIA:
      return "ms_MY";
    case vtkKWLanguage::MALTESE:
      return "mt_MT";
    case vtkKWLanguage::MAORI:
      return "mi";
    case vtkKWLanguage::MARATHI:
      return "mr_IN";
    case vtkKWLanguage::MOLDAVIAN:
      return "mo";
    case vtkKWLanguage::MONGOLIAN:
      return "mn";
    case vtkKWLanguage::NAURU:
      return "na";
    case vtkKWLanguage::NEPALI:
      return "ne";
    case vtkKWLanguage::NEPALI_INDIA:
      return "ne_IN";
    case vtkKWLanguage::NORWEGIAN_BOKMAL:
      return "nb_NO";
    case vtkKWLanguage::NORWEGIAN_NYNORSK:
      return "nn_NO";
    case vtkKWLanguage::OCCITAN:
      return "oc";
    case vtkKWLanguage::ORIYA:
      return "or";
    case vtkKWLanguage::OROMO:
      return "om";
    case vtkKWLanguage::PASHTO:
      return "ps";
    case vtkKWLanguage::POLISH:
      return "pl_PL";
    case vtkKWLanguage::PORTUGUESE:
      return "pt_PT";
    case vtkKWLanguage::PORTUGUESE_BRAZILIAN:
      return "pt_BR";
    case vtkKWLanguage::PUNJABI:
      return "pa";
    case vtkKWLanguage::QUECHUA:
      return "qu";
    case vtkKWLanguage::RHAETO_ROMANCE:
      return "rm";
    case vtkKWLanguage::ROMANIAN:
      return "ro_RO";
    case vtkKWLanguage::RUSSIAN:
      return "ru_RU";
    case vtkKWLanguage::RUSSIAN_UKRAINE:
      return "ru_UA";
    case vtkKWLanguage::SAMOAN:
      return "sm";
    case vtkKWLanguage::SANGHO:
      return "sg";
    case vtkKWLanguage::SANSKRIT:
      return "sa";
    case vtkKWLanguage::SCOTS_GAELIC:
      return "gd";
    case vtkKWLanguage::SERBIAN:
      return "sr_YU";
    case vtkKWLanguage::SERBIAN_CYRILLIC:
      return "sr_YU";
    case vtkKWLanguage::SERBIAN_LATIN:
      return "sr_YU";
    case vtkKWLanguage::SERBO_CROATIAN:
      return "sh";
    case vtkKWLanguage::SESOTHO:
      return "st";
    case vtkKWLanguage::SETSWANA:
      return "tn";
    case vtkKWLanguage::SHONA:
      return "sn";
    case vtkKWLanguage::SINDHI:
      return "sd";
    case vtkKWLanguage::SINHALESE:
      return "si";
    case vtkKWLanguage::SISWATI:
      return "ss";
    case vtkKWLanguage::SLOVAK:
      return "sk_SK";
    case vtkKWLanguage::SLOVENIAN:
      return "sl_SI";
    case vtkKWLanguage::SOMALI:
      return "so";
    case vtkKWLanguage::SPANISH:
      return "es_ES";
    case vtkKWLanguage::SPANISH_ARGENTINA:
      return "es_AR";
    case vtkKWLanguage::SPANISH_BOLIVIA:
      return "es_BO";
    case vtkKWLanguage::SPANISH_CHILE:
      return "es_CL";
    case vtkKWLanguage::SPANISH_COLOMBIA:
      return "es_CO";
    case vtkKWLanguage::SPANISH_COSTA_RICA:
      return "es_CR";
    case vtkKWLanguage::SPANISH_DOMINICAN_REPUBLIC:
      return "es_DO";
    case vtkKWLanguage::SPANISH_ECUADOR:
      return "es_EC";
    case vtkKWLanguage::SPANISH_EL_SALVADOR:
      return "es_SV";
    case vtkKWLanguage::SPANISH_GUATEMALA:
      return "es_GT";
    case vtkKWLanguage::SPANISH_HONDURAS:
      return "es_HN";
    case vtkKWLanguage::SPANISH_MEXICAN:
      return "es_MX";
    case vtkKWLanguage::SPANISH_MODERN:
      return "es_ES";
    case vtkKWLanguage::SPANISH_NICARAGUA:
      return "es_NI";
    case vtkKWLanguage::SPANISH_PANAMA:
      return "es_PA";
    case vtkKWLanguage::SPANISH_PARAGUAY:
      return "es_PY";
    case vtkKWLanguage::SPANISH_PERU:
      return "es_PE";
    case vtkKWLanguage::SPANISH_PUERTO_RICO:
      return "es_PR";
    case vtkKWLanguage::SPANISH_URUGUAY:
      return "es_UY";
    case vtkKWLanguage::SPANISH_US:
      return "es_US";
    case vtkKWLanguage::SPANISH_VENEZUELA:
      return "es_VE";
    case vtkKWLanguage::SUNDANESE:
      return "su";
    case vtkKWLanguage::SWAHILI:
      return "sw_KE";
    case vtkKWLanguage::SWEDISH:
      return "sv_SE";
    case vtkKWLanguage::SWEDISH_FINLAND:
      return "sv_FI";
    case vtkKWLanguage::TAGALOG:
      return "tl_PH";
    case vtkKWLanguage::TAJIK:
      return "tg";
    case vtkKWLanguage::TAMIL:
      return "ta";
    case vtkKWLanguage::TATAR:
      return "tt";
    case vtkKWLanguage::TELUGU:
      return "te";
    case vtkKWLanguage::THAI:
      return "th_TH";
    case vtkKWLanguage::TIBETAN:
      return "bo";
    case vtkKWLanguage::TIGRINYA:
      return "ti";
    case vtkKWLanguage::TONGA:
      return "to";
    case vtkKWLanguage::TSONGA:
      return "ts";
    case vtkKWLanguage::TURKISH:
      return "tr_TR";
    case vtkKWLanguage::TURKMEN:
      return "tk";
    case vtkKWLanguage::TWI:
      return "tw";
    case vtkKWLanguage::UIGHUR:
      return "ug";
    case vtkKWLanguage::UKRAINIAN:
      return "uk_UA";
    case vtkKWLanguage::URDU:
      return "ur";
    case vtkKWLanguage::URDU_INDIA:
      return "ur_IN";
    case vtkKWLanguage::URDU_PAKISTAN:
      return "ur_PK";
    case vtkKWLanguage::UZBEK:
      return "uz";
    case vtkKWLanguage::UZBEK_CYRILLIC:
      return "uz";
    case vtkKWLanguage::UZBEK_LATIN:
      return "uz";
    case vtkKWLanguage::VIETNAMESE:
      return "vi_VN";
    case vtkKWLanguage::VOLAPUK:
      return "vo";
    case vtkKWLanguage::WELSH:
      return "cy";
    case vtkKWLanguage::WOLOF:
      return "wo";
    case vtkKWLanguage::XHOSA:
      return "xh";
    case vtkKWLanguage::YIDDISH:
      return "yi";
    case vtkKWLanguage::YORUBA:
      return "yo";
    case vtkKWLanguage::ZHUANG:
      return "za";
    case vtkKWLanguage::ZULU:
      return "zu";

    default: 
      return NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWLanguage::GetLanguageFromXPG(const char *xpg)
{
  int lang = vtkKWLanguage::GetLanguageFromXPGStrict(xpg);
  if (lang != vtkKWLanguage::UNKNOWN)
    {
    return lang;
    }

  // Try by adding a territory matching the language
  // This allows 'fr' to match 'fr_FR'

  if (xpg && strlen(xpg) == 2)
    {
    char xpg_long[6]; 
    xpg_long[0] = xpg[0];
    xpg_long[1] = xpg[1];
    xpg_long[2] = '_';
    xpg_long[3] = toupper(xpg[0]);
    xpg_long[4] = toupper(xpg[1]);
    xpg_long[5] = '\0';
    return vtkKWLanguage::GetLanguageFromXPGStrict(xpg_long);
    }

  return vtkKWLanguage::UNKNOWN;
}


//----------------------------------------------------------------------------
int vtkKWLanguage::GetLanguageFromXPGStrict(const char *xpg)
{
  if (!xpg || !*xpg)
    {
    return vtkKWLanguage::UNKNOWN;
    }

  // Not very efficient but was generated by a script

  if (!strcmp(xpg, "ab"))
    return vtkKWLanguage::ABKHAZIAN;
  if (!strcmp(xpg, "aa"))
    return vtkKWLanguage::AFAR;
  if (!strcmp(xpg, "af_ZA"))
    return vtkKWLanguage::AFRIKAANS;
  if (!strcmp(xpg, "sq_AL"))
    return vtkKWLanguage::ALBANIAN;
  if (!strcmp(xpg, "am"))
    return vtkKWLanguage::AMHARIC;
  if (!strcmp(xpg, "ar"))
    return vtkKWLanguage::ARABIC;
  if (!strcmp(xpg, "ar_DZ"))
    return vtkKWLanguage::ARABIC_ALGERIA;
  if (!strcmp(xpg, "ar_BH"))
    return vtkKWLanguage::ARABIC_BAHRAIN;
  if (!strcmp(xpg, "ar_EG"))
    return vtkKWLanguage::ARABIC_EGYPT;
  if (!strcmp(xpg, "ar_IQ"))
    return vtkKWLanguage::ARABIC_IRAQ;
  if (!strcmp(xpg, "ar_JO"))
    return vtkKWLanguage::ARABIC_JORDAN;
  if (!strcmp(xpg, "ar_KW"))
    return vtkKWLanguage::ARABIC_KUWAIT;
  if (!strcmp(xpg, "ar_LB"))
    return vtkKWLanguage::ARABIC_LEBANON;
  if (!strcmp(xpg, "ar_LY"))
    return vtkKWLanguage::ARABIC_LIBYA;
  if (!strcmp(xpg, "ar_MA"))
    return vtkKWLanguage::ARABIC_MOROCCO;
  if (!strcmp(xpg, "ar_OM"))
    return vtkKWLanguage::ARABIC_OMAN;
  if (!strcmp(xpg, "ar_QA"))
    return vtkKWLanguage::ARABIC_QATAR;
  if (!strcmp(xpg, "ar_SA"))
    return vtkKWLanguage::ARABIC_SAUDI_ARABIA;
  if (!strcmp(xpg, "ar_SD"))
    return vtkKWLanguage::ARABIC_SUDAN;
  if (!strcmp(xpg, "ar_SY"))
    return vtkKWLanguage::ARABIC_SYRIA;
  if (!strcmp(xpg, "ar_TN"))
    return vtkKWLanguage::ARABIC_TUNISIA;
  if (!strcmp(xpg, "ar_AE"))
    return vtkKWLanguage::ARABIC_UAE;
  if (!strcmp(xpg, "ar_YE"))
    return vtkKWLanguage::ARABIC_YEMEN;
  if (!strcmp(xpg, "hy"))
    return vtkKWLanguage::ARMENIAN;
  if (!strcmp(xpg, "as"))
    return vtkKWLanguage::ASSAMESE;
  if (!strcmp(xpg, "ay"))
    return vtkKWLanguage::AYMARA;
  if (!strcmp(xpg, "az"))
    return vtkKWLanguage::AZERI;
  if (!strcmp(xpg, "az"))
    return vtkKWLanguage::AZERI_CYRILLIC;
  if (!strcmp(xpg, "az"))
    return vtkKWLanguage::AZERI_LATIN;
  if (!strcmp(xpg, "ba"))
    return vtkKWLanguage::BASHKIR;
  if (!strcmp(xpg, "eu_ES"))
    return vtkKWLanguage::BASQUE;
  if (!strcmp(xpg, "be_BY"))
    return vtkKWLanguage::BELARUSIAN;
  if (!strcmp(xpg, "bn"))
    return vtkKWLanguage::BENGALI;
  if (!strcmp(xpg, "dz"))
    return vtkKWLanguage::BHUTANI;
  if (!strcmp(xpg, "bh"))
    return vtkKWLanguage::BIHARI;
  if (!strcmp(xpg, "bi"))
    return vtkKWLanguage::BISLAMA;
  if (!strcmp(xpg, "br"))
    return vtkKWLanguage::BRETON;
  if (!strcmp(xpg, "bg_BG"))
    return vtkKWLanguage::BULGARIAN;
  if (!strcmp(xpg, "my"))
    return vtkKWLanguage::BURMESE;
  if (!strcmp(xpg, "km"))
    return vtkKWLanguage::CAMBODIAN;
  if (!strcmp(xpg, "ca_ES"))
    return vtkKWLanguage::CATALAN;
  if (!strcmp(xpg, "zh_TW"))
    return vtkKWLanguage::CHINESE;
  if (!strcmp(xpg, "zh_CN"))
    return vtkKWLanguage::CHINESE_SIMPLIFIED;
  if (!strcmp(xpg, "zh_TW"))
    return vtkKWLanguage::CHINESE_TRADITIONAL;
  if (!strcmp(xpg, "zh_HK"))
    return vtkKWLanguage::CHINESE_HONGKONG;
  if (!strcmp(xpg, "zh_MO"))
    return vtkKWLanguage::CHINESE_MACAU;
  if (!strcmp(xpg, "zh_SG"))
    return vtkKWLanguage::CHINESE_SINGAPORE;
  if (!strcmp(xpg, "zh_TW"))
    return vtkKWLanguage::CHINESE_TAIWAN;
  if (!strcmp(xpg, "co"))
    return vtkKWLanguage::CORSICAN;
  if (!strcmp(xpg, "hr_HR"))
    return vtkKWLanguage::CROATIAN;
  if (!strcmp(xpg, "cs_CZ"))
    return vtkKWLanguage::CZECH;
  if (!strcmp(xpg, "da_DK"))
    return vtkKWLanguage::DANISH;
  if (!strcmp(xpg, "nl_NL"))
    return vtkKWLanguage::DUTCH;
  if (!strcmp(xpg, "nl_BE"))
    return vtkKWLanguage::DUTCH_BELGIAN;
  if (!strcmp(xpg, "en_GB"))
    return vtkKWLanguage::ENGLISH;
  if (!strcmp(xpg, "en_GB"))
    return vtkKWLanguage::ENGLISH_UK;
  if (!strcmp(xpg, "en_US"))
    return vtkKWLanguage::ENGLISH_US;
  if (!strcmp(xpg, "en_AU"))
    return vtkKWLanguage::ENGLISH_AUSTRALIA;
  if (!strcmp(xpg, "en_BZ"))
    return vtkKWLanguage::ENGLISH_BELIZE;
  if (!strcmp(xpg, "en_BW"))
    return vtkKWLanguage::ENGLISH_BOTSWANA;
  if (!strcmp(xpg, "en_CA"))
    return vtkKWLanguage::ENGLISH_CANADA;
  if (!strcmp(xpg, "en_CB"))
    return vtkKWLanguage::ENGLISH_CARIBBEAN;
  if (!strcmp(xpg, "en_DK"))
    return vtkKWLanguage::ENGLISH_DENMARK;
  if (!strcmp(xpg, "en_IE"))
    return vtkKWLanguage::ENGLISH_EIRE;
  if (!strcmp(xpg, "en_JM"))
    return vtkKWLanguage::ENGLISH_JAMAICA;
  if (!strcmp(xpg, "en_NZ"))
    return vtkKWLanguage::ENGLISH_NEW_ZEALAND;
  if (!strcmp(xpg, "en_PH"))
    return vtkKWLanguage::ENGLISH_PHILIPPINES;
  if (!strcmp(xpg, "en_ZA"))
    return vtkKWLanguage::ENGLISH_SOUTH_AFRICA;
  if (!strcmp(xpg, "en_TT"))
    return vtkKWLanguage::ENGLISH_TRINIDAD;
  if (!strcmp(xpg, "en_ZW"))
    return vtkKWLanguage::ENGLISH_ZIMBABWE;
  if (!strcmp(xpg, "eo"))
    return vtkKWLanguage::ESPERANTO;
  if (!strcmp(xpg, "et_EE"))
    return vtkKWLanguage::ESTONIAN;
  if (!strcmp(xpg, "fo_FO"))
    return vtkKWLanguage::FAEROESE;
  if (!strcmp(xpg, "fa_IR"))
    return vtkKWLanguage::FARSI;
  if (!strcmp(xpg, "fj"))
    return vtkKWLanguage::FIJI;
  if (!strcmp(xpg, "fi_FI"))
    return vtkKWLanguage::FINNISH;
  if (!strcmp(xpg, "fr_FR"))
    return vtkKWLanguage::FRENCH;
  if (!strcmp(xpg, "fr_BE"))
    return vtkKWLanguage::FRENCH_BELGIAN;
  if (!strcmp(xpg, "fr_CA"))
    return vtkKWLanguage::FRENCH_CANADIAN;
  if (!strcmp(xpg, "fr_LU"))
    return vtkKWLanguage::FRENCH_LUXEMBOURG;
  if (!strcmp(xpg, "fr_MC"))
    return vtkKWLanguage::FRENCH_MONACO;
  if (!strcmp(xpg, "fr_CH"))
    return vtkKWLanguage::FRENCH_SWISS;
  if (!strcmp(xpg, "fy"))
    return vtkKWLanguage::FRISIAN;
  if (!strcmp(xpg, "gl_ES"))
    return vtkKWLanguage::GALICIAN;
  if (!strcmp(xpg, "ka"))
    return vtkKWLanguage::GEORGIAN;
  if (!strcmp(xpg, "de_DE"))
    return vtkKWLanguage::GERMAN;
  if (!strcmp(xpg, "de_AT"))
    return vtkKWLanguage::GERMAN_AUSTRIAN;
  if (!strcmp(xpg, "de_BE"))
    return vtkKWLanguage::GERMAN_BELGIUM;
  if (!strcmp(xpg, "de_LI"))
    return vtkKWLanguage::GERMAN_LIECHTENSTEIN;
  if (!strcmp(xpg, "de_LU"))
    return vtkKWLanguage::GERMAN_LUXEMBOURG;
  if (!strcmp(xpg, "de_CH"))
    return vtkKWLanguage::GERMAN_SWISS;
  if (!strcmp(xpg, "el_GR"))
    return vtkKWLanguage::GREEK;
  if (!strcmp(xpg, "kl_GL"))
    return vtkKWLanguage::GREENLANDIC;
  if (!strcmp(xpg, "gn"))
    return vtkKWLanguage::GUARANI;
  if (!strcmp(xpg, "gu"))
    return vtkKWLanguage::GUJARATI;
  if (!strcmp(xpg, "ha"))
    return vtkKWLanguage::HAUSA;
  if (!strcmp(xpg, "he_IL"))
    return vtkKWLanguage::HEBREW;
  if (!strcmp(xpg, "hi_IN"))
    return vtkKWLanguage::HINDI;
  if (!strcmp(xpg, "hu_HU"))
    return vtkKWLanguage::HUNGARIAN;
  if (!strcmp(xpg, "is_IS"))
    return vtkKWLanguage::ICELANDIC;
  if (!strcmp(xpg, "id_ID"))
    return vtkKWLanguage::INDONESIAN;
  if (!strcmp(xpg, "ia"))
    return vtkKWLanguage::INTERLINGUA;
  if (!strcmp(xpg, "ie"))
    return vtkKWLanguage::INTERLINGUE;
  if (!strcmp(xpg, "iu"))
    return vtkKWLanguage::INUKTITUT;
  if (!strcmp(xpg, "ik"))
    return vtkKWLanguage::INUPIAK;
  if (!strcmp(xpg, "ga_IE"))
    return vtkKWLanguage::IRISH;
  if (!strcmp(xpg, "it_IT"))
    return vtkKWLanguage::ITALIAN;
  if (!strcmp(xpg, "it_CH"))
    return vtkKWLanguage::ITALIAN_SWISS;
  if (!strcmp(xpg, "ja_JP"))
    return vtkKWLanguage::JAPANESE;
  if (!strcmp(xpg, "jw"))
    return vtkKWLanguage::JAVANESE;
  if (!strcmp(xpg, "kn"))
    return vtkKWLanguage::KANNADA;
  if (!strcmp(xpg, "ks"))
    return vtkKWLanguage::KASHMIRI;
  if (!strcmp(xpg, "ks_IN"))
    return vtkKWLanguage::KASHMIRI_INDIA;
  if (!strcmp(xpg, "kk"))
    return vtkKWLanguage::KAZAKH;
  if (!strcmp(xpg, "kw_GB"))
    return vtkKWLanguage::KERNEWEK;
  if (!strcmp(xpg, "rw"))
    return vtkKWLanguage::KINYARWANDA;
  if (!strcmp(xpg, "ky"))
    return vtkKWLanguage::KIRGHIZ;
  if (!strcmp(xpg, "rn"))
    return vtkKWLanguage::KIRUNDI;
  if (!strcmp(xpg, "ko_KR"))
    return vtkKWLanguage::KOREAN;
  if (!strcmp(xpg, "ku"))
    return vtkKWLanguage::KURDISH;
  if (!strcmp(xpg, "lo"))
    return vtkKWLanguage::LAOTHIAN;
  if (!strcmp(xpg, "la"))
    return vtkKWLanguage::LATIN;
  if (!strcmp(xpg, "lv_LV"))
    return vtkKWLanguage::LATVIAN;
  if (!strcmp(xpg, "ln"))
    return vtkKWLanguage::LINGALA;
  if (!strcmp(xpg, "lt_LT"))
    return vtkKWLanguage::LITHUANIAN;
  if (!strcmp(xpg, "mk_MK"))
    return vtkKWLanguage::MACEDONIAN;
  if (!strcmp(xpg, "mg"))
    return vtkKWLanguage::MALAGASY;
  if (!strcmp(xpg, "ms_MY"))
    return vtkKWLanguage::MALAY;
  if (!strcmp(xpg, "ml"))
    return vtkKWLanguage::MALAYALAM;
  if (!strcmp(xpg, "ms_BN"))
    return vtkKWLanguage::MALAY_BRUNEI_DARUSSALAM;
  if (!strcmp(xpg, "ms_MY"))
    return vtkKWLanguage::MALAY_MALAYSIA;
  if (!strcmp(xpg, "mt_MT"))
    return vtkKWLanguage::MALTESE;
  if (!strcmp(xpg, "mi"))
    return vtkKWLanguage::MAORI;
  if (!strcmp(xpg, "mr_IN"))
    return vtkKWLanguage::MARATHI;
  if (!strcmp(xpg, "mo"))
    return vtkKWLanguage::MOLDAVIAN;
  if (!strcmp(xpg, "mn"))
    return vtkKWLanguage::MONGOLIAN;
  if (!strcmp(xpg, "na"))
    return vtkKWLanguage::NAURU;
  if (!strcmp(xpg, "ne"))
    return vtkKWLanguage::NEPALI;
  if (!strcmp(xpg, "ne_IN"))
    return vtkKWLanguage::NEPALI_INDIA;
  if (!strcmp(xpg, "nb_NO"))
    return vtkKWLanguage::NORWEGIAN_BOKMAL;
  if (!strcmp(xpg, "nn_NO"))
    return vtkKWLanguage::NORWEGIAN_NYNORSK;
  if (!strcmp(xpg, "oc"))
    return vtkKWLanguage::OCCITAN;
  if (!strcmp(xpg, "or"))
    return vtkKWLanguage::ORIYA;
  if (!strcmp(xpg, "om"))
    return vtkKWLanguage::OROMO;
  if (!strcmp(xpg, "ps"))
    return vtkKWLanguage::PASHTO;
  if (!strcmp(xpg, "pl_PL"))
    return vtkKWLanguage::POLISH;
  if (!strcmp(xpg, "pt_PT"))
    return vtkKWLanguage::PORTUGUESE;
  if (!strcmp(xpg, "pt_BR"))
    return vtkKWLanguage::PORTUGUESE_BRAZILIAN;
  if (!strcmp(xpg, "pa"))
    return vtkKWLanguage::PUNJABI;
  if (!strcmp(xpg, "qu"))
    return vtkKWLanguage::QUECHUA;
  if (!strcmp(xpg, "rm"))
    return vtkKWLanguage::RHAETO_ROMANCE;
  if (!strcmp(xpg, "ro_RO"))
    return vtkKWLanguage::ROMANIAN;
  if (!strcmp(xpg, "ru_RU"))
    return vtkKWLanguage::RUSSIAN;
  if (!strcmp(xpg, "ru_UA"))
    return vtkKWLanguage::RUSSIAN_UKRAINE;
  if (!strcmp(xpg, "sm"))
    return vtkKWLanguage::SAMOAN;
  if (!strcmp(xpg, "sg"))
    return vtkKWLanguage::SANGHO;
  if (!strcmp(xpg, "sa"))
    return vtkKWLanguage::SANSKRIT;
  if (!strcmp(xpg, "gd"))
    return vtkKWLanguage::SCOTS_GAELIC;
  if (!strcmp(xpg, "sr_YU"))
    return vtkKWLanguage::SERBIAN;
  if (!strcmp(xpg, "sr_YU"))
    return vtkKWLanguage::SERBIAN_CYRILLIC;
  if (!strcmp(xpg, "sr_YU"))
    return vtkKWLanguage::SERBIAN_LATIN;
  if (!strcmp(xpg, "sh"))
    return vtkKWLanguage::SERBO_CROATIAN;
  if (!strcmp(xpg, "st"))
    return vtkKWLanguage::SESOTHO;
  if (!strcmp(xpg, "tn"))
    return vtkKWLanguage::SETSWANA;
  if (!strcmp(xpg, "sn"))
    return vtkKWLanguage::SHONA;
  if (!strcmp(xpg, "sd"))
    return vtkKWLanguage::SINDHI;
  if (!strcmp(xpg, "si"))
    return vtkKWLanguage::SINHALESE;
  if (!strcmp(xpg, "ss"))
    return vtkKWLanguage::SISWATI;
  if (!strcmp(xpg, "sk_SK"))
    return vtkKWLanguage::SLOVAK;
  if (!strcmp(xpg, "sl_SI"))
    return vtkKWLanguage::SLOVENIAN;
  if (!strcmp(xpg, "so"))
    return vtkKWLanguage::SOMALI;
  if (!strcmp(xpg, "es_ES"))
    return vtkKWLanguage::SPANISH;
  if (!strcmp(xpg, "es_AR"))
    return vtkKWLanguage::SPANISH_ARGENTINA;
  if (!strcmp(xpg, "es_BO"))
    return vtkKWLanguage::SPANISH_BOLIVIA;
  if (!strcmp(xpg, "es_CL"))
    return vtkKWLanguage::SPANISH_CHILE;
  if (!strcmp(xpg, "es_CO"))
    return vtkKWLanguage::SPANISH_COLOMBIA;
  if (!strcmp(xpg, "es_CR"))
    return vtkKWLanguage::SPANISH_COSTA_RICA;
  if (!strcmp(xpg, "es_DO"))
    return vtkKWLanguage::SPANISH_DOMINICAN_REPUBLIC;
  if (!strcmp(xpg, "es_EC"))
    return vtkKWLanguage::SPANISH_ECUADOR;
  if (!strcmp(xpg, "es_SV"))
    return vtkKWLanguage::SPANISH_EL_SALVADOR;
  if (!strcmp(xpg, "es_GT"))
    return vtkKWLanguage::SPANISH_GUATEMALA;
  if (!strcmp(xpg, "es_HN"))
    return vtkKWLanguage::SPANISH_HONDURAS;
  if (!strcmp(xpg, "es_MX"))
    return vtkKWLanguage::SPANISH_MEXICAN;
  if (!strcmp(xpg, "es_ES"))
    return vtkKWLanguage::SPANISH_MODERN;
  if (!strcmp(xpg, "es_NI"))
    return vtkKWLanguage::SPANISH_NICARAGUA;
  if (!strcmp(xpg, "es_PA"))
    return vtkKWLanguage::SPANISH_PANAMA;
  if (!strcmp(xpg, "es_PY"))
    return vtkKWLanguage::SPANISH_PARAGUAY;
  if (!strcmp(xpg, "es_PE"))
    return vtkKWLanguage::SPANISH_PERU;
  if (!strcmp(xpg, "es_PR"))
    return vtkKWLanguage::SPANISH_PUERTO_RICO;
  if (!strcmp(xpg, "es_UY"))
    return vtkKWLanguage::SPANISH_URUGUAY;
  if (!strcmp(xpg, "es_US"))
    return vtkKWLanguage::SPANISH_US;
  if (!strcmp(xpg, "es_VE"))
    return vtkKWLanguage::SPANISH_VENEZUELA;
  if (!strcmp(xpg, "su"))
    return vtkKWLanguage::SUNDANESE;
  if (!strcmp(xpg, "sw_KE"))
    return vtkKWLanguage::SWAHILI;
  if (!strcmp(xpg, "sv_SE"))
    return vtkKWLanguage::SWEDISH;
  if (!strcmp(xpg, "sv_FI"))
    return vtkKWLanguage::SWEDISH_FINLAND;
  if (!strcmp(xpg, "tl_PH"))
    return vtkKWLanguage::TAGALOG;
  if (!strcmp(xpg, "tg"))
    return vtkKWLanguage::TAJIK;
  if (!strcmp(xpg, "ta"))
    return vtkKWLanguage::TAMIL;
  if (!strcmp(xpg, "tt"))
    return vtkKWLanguage::TATAR;
  if (!strcmp(xpg, "te"))
    return vtkKWLanguage::TELUGU;
  if (!strcmp(xpg, "th_TH"))
    return vtkKWLanguage::THAI;
  if (!strcmp(xpg, "bo"))
    return vtkKWLanguage::TIBETAN;
  if (!strcmp(xpg, "ti"))
    return vtkKWLanguage::TIGRINYA;
  if (!strcmp(xpg, "to"))
    return vtkKWLanguage::TONGA;
  if (!strcmp(xpg, "ts"))
    return vtkKWLanguage::TSONGA;
  if (!strcmp(xpg, "tr_TR"))
    return vtkKWLanguage::TURKISH;
  if (!strcmp(xpg, "tk"))
    return vtkKWLanguage::TURKMEN;
  if (!strcmp(xpg, "tw"))
    return vtkKWLanguage::TWI;
  if (!strcmp(xpg, "ug"))
    return vtkKWLanguage::UIGHUR;
  if (!strcmp(xpg, "uk_UA"))
    return vtkKWLanguage::UKRAINIAN;
  if (!strcmp(xpg, "ur"))
    return vtkKWLanguage::URDU;
  if (!strcmp(xpg, "ur_IN"))
    return vtkKWLanguage::URDU_INDIA;
  if (!strcmp(xpg, "ur_PK"))
    return vtkKWLanguage::URDU_PAKISTAN;
  if (!strcmp(xpg, "uz"))
    return vtkKWLanguage::UZBEK;
  if (!strcmp(xpg, "uz"))
    return vtkKWLanguage::UZBEK_CYRILLIC;
  if (!strcmp(xpg, "uz"))
    return vtkKWLanguage::UZBEK_LATIN;
  if (!strcmp(xpg, "vi_VN"))
    return vtkKWLanguage::VIETNAMESE;
  if (!strcmp(xpg, "vo"))
    return vtkKWLanguage::VOLAPUK;
  if (!strcmp(xpg, "cy"))
    return vtkKWLanguage::WELSH;
  if (!strcmp(xpg, "wo"))
    return vtkKWLanguage::WOLOF;
  if (!strcmp(xpg, "xh"))
    return vtkKWLanguage::XHOSA;
  if (!strcmp(xpg, "yi"))
    return vtkKWLanguage::YIDDISH;
  if (!strcmp(xpg, "yo"))
    return vtkKWLanguage::YORUBA;
  if (!strcmp(xpg, "za"))
    return vtkKWLanguage::ZHUANG;
  if (!strcmp(xpg, "zu"))
    return vtkKWLanguage::ZULU;
  
  return vtkKWLanguage::UNKNOWN;
}

//----------------------------------------------------------------------------
int vtkKWLanguage::GetWin32LANGIDFromLanguage(int lang)
{
#if _WIN32
  switch (lang)
    {
    case vtkKWLanguage::AFRIKAANS:
      return (int)MAKELANGID(LANG_AFRIKAANS, SUBLANG_DEFAULT);
    case vtkKWLanguage::ALBANIAN:
      return (int)MAKELANGID(LANG_ALBANIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::ARABIC:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT);
    case vtkKWLanguage::ARABIC_ALGERIA:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_ALGERIA);
    case vtkKWLanguage::ARABIC_BAHRAIN:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_BAHRAIN);
    case vtkKWLanguage::ARABIC_EGYPT:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_EGYPT);
    case vtkKWLanguage::ARABIC_IRAQ:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_IRAQ);
    case vtkKWLanguage::ARABIC_JORDAN:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_JORDAN);
    case vtkKWLanguage::ARABIC_KUWAIT:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_KUWAIT);
    case vtkKWLanguage::ARABIC_LEBANON:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_LEBANON);
    case vtkKWLanguage::ARABIC_LIBYA:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_LIBYA);
    case vtkKWLanguage::ARABIC_MOROCCO:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_MOROCCO);
    case vtkKWLanguage::ARABIC_OMAN:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_OMAN);
    case vtkKWLanguage::ARABIC_QATAR:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_QATAR);
    case vtkKWLanguage::ARABIC_SAUDI_ARABIA:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
    case vtkKWLanguage::ARABIC_SYRIA:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SYRIA);
    case vtkKWLanguage::ARABIC_TUNISIA:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_TUNISIA);
    case vtkKWLanguage::ARABIC_UAE:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_UAE);
    case vtkKWLanguage::ARABIC_YEMEN:
      return (int)MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_YEMEN);
    case vtkKWLanguage::ARMENIAN:
      return (int)MAKELANGID(LANG_ARMENIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::ASSAMESE:
      return (int)MAKELANGID(LANG_ASSAMESE, SUBLANG_DEFAULT);
    case vtkKWLanguage::AZERI:
      return (int)MAKELANGID(LANG_AZERI, SUBLANG_DEFAULT);
    case vtkKWLanguage::AZERI_CYRILLIC:
      return (int)MAKELANGID(LANG_AZERI, SUBLANG_AZERI_CYRILLIC);
    case vtkKWLanguage::AZERI_LATIN:
      return (int)MAKELANGID(LANG_AZERI, SUBLANG_AZERI_LATIN);
    case vtkKWLanguage::BASQUE:
      return (int)MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT);
    case vtkKWLanguage::BELARUSIAN:
      return (int)MAKELANGID(LANG_BELARUSIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::BENGALI:
      return (int)MAKELANGID(LANG_BENGALI, SUBLANG_DEFAULT);
    case vtkKWLanguage::BULGARIAN:
      return (int)MAKELANGID(LANG_BULGARIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::CATALAN:
      return (int)MAKELANGID(LANG_CATALAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::CHINESE:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_DEFAULT);
    case vtkKWLanguage::CHINESE_SIMPLIFIED:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    case vtkKWLanguage::CHINESE_TRADITIONAL:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
    case vtkKWLanguage::CHINESE_HONGKONG:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG);
    case vtkKWLanguage::CHINESE_MACAU:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_MACAU);
    case vtkKWLanguage::CHINESE_SINGAPORE:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE);
    case vtkKWLanguage::CHINESE_TAIWAN:
      return (int)MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
    case vtkKWLanguage::CROATIAN:
      return (int)MAKELANGID(LANG_CROATIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::CZECH:
      return (int)MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT);
    case vtkKWLanguage::DANISH:
      return (int)MAKELANGID(LANG_DANISH, SUBLANG_DEFAULT);
    case vtkKWLanguage::DUTCH:
      return (int)MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH);
    case vtkKWLanguage::DUTCH_BELGIAN:
      return (int)MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN);
    case vtkKWLanguage::ENGLISH:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK);
    case vtkKWLanguage::ENGLISH_UK:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK);
    case vtkKWLanguage::ENGLISH_US:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    case vtkKWLanguage::ENGLISH_AUSTRALIA:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_AUS);
    case vtkKWLanguage::ENGLISH_BELIZE:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_BELIZE);
    case vtkKWLanguage::ENGLISH_CANADA:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_CAN);
    case vtkKWLanguage::ENGLISH_CARIBBEAN:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_CARIBBEAN);
    case vtkKWLanguage::ENGLISH_EIRE:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_EIRE);
    case vtkKWLanguage::ENGLISH_JAMAICA:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_JAMAICA);
    case vtkKWLanguage::ENGLISH_NEW_ZEALAND:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_NZ);
    case vtkKWLanguage::ENGLISH_PHILIPPINES:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_PHILIPPINES);
    case vtkKWLanguage::ENGLISH_SOUTH_AFRICA:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_SOUTH_AFRICA);
    case vtkKWLanguage::ENGLISH_TRINIDAD:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_TRINIDAD);
    case vtkKWLanguage::ENGLISH_ZIMBABWE:
      return (int)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_ZIMBABWE);
    case vtkKWLanguage::ESTONIAN:
      return (int)MAKELANGID(LANG_ESTONIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::FAEROESE:
      return (int)MAKELANGID(LANG_FAEROESE, SUBLANG_DEFAULT);
    case vtkKWLanguage::FARSI:
      return (int)MAKELANGID(LANG_FARSI, SUBLANG_DEFAULT);
    case vtkKWLanguage::FINNISH:
      return (int)MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT);
    case vtkKWLanguage::FRENCH:
      return (int)MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
    case vtkKWLanguage::FRENCH_BELGIAN:
      return (int)MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_BELGIAN);
    case vtkKWLanguage::FRENCH_CANADIAN:
      return (int)MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_CANADIAN);
    case vtkKWLanguage::FRENCH_LUXEMBOURG:
      return (int)MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_LUXEMBOURG);
    case vtkKWLanguage::FRENCH_MONACO:
      return (int)MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_MONACO);
    case vtkKWLanguage::FRENCH_SWISS:
      return (int)MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_SWISS);
    case vtkKWLanguage::GEORGIAN:
      return (int)MAKELANGID(LANG_GEORGIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::GERMAN:
      return (int)MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
    case vtkKWLanguage::GERMAN_AUSTRIAN:
      return (int)MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_AUSTRIAN);
    case vtkKWLanguage::GERMAN_LIECHTENSTEIN:
      return (int)MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_LIECHTENSTEIN);
    case vtkKWLanguage::GERMAN_LUXEMBOURG:
      return (int)MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_LUXEMBOURG);
    case vtkKWLanguage::GERMAN_SWISS:
      return (int)MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_SWISS);
    case vtkKWLanguage::GREEK:
      return (int)MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT);
    case vtkKWLanguage::GUJARATI:
      return (int)MAKELANGID(LANG_GUJARATI, SUBLANG_DEFAULT);
    case vtkKWLanguage::HEBREW:
      return (int)MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT);
    case vtkKWLanguage::HINDI:
      return (int)MAKELANGID(LANG_HINDI, SUBLANG_DEFAULT);
    case vtkKWLanguage::HUNGARIAN:
      return (int)MAKELANGID(LANG_HUNGARIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::ICELANDIC:
      return (int)MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT);
    case vtkKWLanguage::INDONESIAN:
      return (int)MAKELANGID(LANG_INDONESIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::ITALIAN:
      return (int)MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
    case vtkKWLanguage::ITALIAN_SWISS:
      return (int)MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN_SWISS);
    case vtkKWLanguage::JAPANESE:
      return (int)MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
    case vtkKWLanguage::KANNADA:
      return (int)MAKELANGID(LANG_KANNADA, SUBLANG_DEFAULT);
    case vtkKWLanguage::KASHMIRI:
      return (int)MAKELANGID(LANG_KASHMIRI, SUBLANG_DEFAULT);
    case vtkKWLanguage::KASHMIRI_INDIA:
      return (int)MAKELANGID(LANG_KASHMIRI, SUBLANG_KASHMIRI_INDIA);
    case vtkKWLanguage::KAZAKH:
      return (int)MAKELANGID(LANG_KAZAK, SUBLANG_DEFAULT);
    case vtkKWLanguage::KONKANI:
      return (int)MAKELANGID(LANG_KONKANI, SUBLANG_DEFAULT);
    case vtkKWLanguage::KOREAN:
      return (int)MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
    case vtkKWLanguage::LATVIAN:
      return (int)MAKELANGID(LANG_LATVIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::LITHUANIAN:
      return (int)MAKELANGID(LANG_LITHUANIAN, SUBLANG_LITHUANIAN);
    case vtkKWLanguage::MACEDONIAN:
      return (int)MAKELANGID(LANG_MACEDONIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::MALAY:
      return (int)MAKELANGID(LANG_MALAY, SUBLANG_DEFAULT);
    case vtkKWLanguage::MALAYALAM:
      return (int)MAKELANGID(LANG_MALAYALAM, SUBLANG_DEFAULT);
    case vtkKWLanguage::MALAY_BRUNEI_DARUSSALAM:
      return (int)MAKELANGID(LANG_MALAY, SUBLANG_MALAY_BRUNEI_DARUSSALAM);
    case vtkKWLanguage::MALAY_MALAYSIA:
      return (int)MAKELANGID(LANG_MALAY, SUBLANG_MALAY_MALAYSIA);
    case vtkKWLanguage::MANIPURI:
      return (int)MAKELANGID(LANG_MANIPURI, SUBLANG_DEFAULT);
    case vtkKWLanguage::MARATHI:
      return (int)MAKELANGID(LANG_MARATHI, SUBLANG_DEFAULT);
    case vtkKWLanguage::NEPALI:
      return (int)MAKELANGID(LANG_NEPALI, SUBLANG_DEFAULT);
    case vtkKWLanguage::NEPALI_INDIA:
      return (int)MAKELANGID(LANG_NEPALI, SUBLANG_NEPALI_INDIA);
    case vtkKWLanguage::NORWEGIAN_BOKMAL:
      return (int)MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL);
    case vtkKWLanguage::NORWEGIAN_NYNORSK:
      return (int)MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK);
    case vtkKWLanguage::ORIYA:
      return (int)MAKELANGID(LANG_ORIYA, SUBLANG_DEFAULT);
    case vtkKWLanguage::POLISH:
      return (int)MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT);
    case vtkKWLanguage::PORTUGUESE:
      return (int)MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE);
    case vtkKWLanguage::PORTUGUESE_BRAZILIAN:
      return (int)MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN);
    case vtkKWLanguage::PUNJABI:
      return (int)MAKELANGID(LANG_PUNJABI, SUBLANG_DEFAULT);
    case vtkKWLanguage::ROMANIAN:
      return (int)MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::RUSSIAN:
      return (int)MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::SANSKRIT:
      return (int)MAKELANGID(LANG_SANSKRIT, SUBLANG_DEFAULT);
    case vtkKWLanguage::SERBIAN:
      return (int)MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::SERBIAN_CYRILLIC:
      return (int)MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC);
    case vtkKWLanguage::SERBIAN_LATIN:
      return (int)MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_LATIN);
    case vtkKWLanguage::SINDHI:
      return (int)MAKELANGID(LANG_SINDHI, SUBLANG_DEFAULT);
    case vtkKWLanguage::SLOVAK:
      return (int)MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT);
    case vtkKWLanguage::SLOVENIAN:
      return (int)MAKELANGID(LANG_SLOVENIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::SPANISH:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
    case vtkKWLanguage::SPANISH_ARGENTINA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_ARGENTINA);
    case vtkKWLanguage::SPANISH_BOLIVIA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_BOLIVIA);
    case vtkKWLanguage::SPANISH_CHILE:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_CHILE);
    case vtkKWLanguage::SPANISH_COLOMBIA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_COLOMBIA);
    case vtkKWLanguage::SPANISH_COSTA_RICA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_COSTA_RICA);
    case vtkKWLanguage::SPANISH_DOMINICAN_REPUBLIC:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_DOMINICAN_REPUBLIC);
    case vtkKWLanguage::SPANISH_ECUADOR:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_ECUADOR);
    case vtkKWLanguage::SPANISH_EL_SALVADOR:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_EL_SALVADOR);
    case vtkKWLanguage::SPANISH_GUATEMALA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_GUATEMALA);
    case vtkKWLanguage::SPANISH_HONDURAS:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_HONDURAS);
    case vtkKWLanguage::SPANISH_MEXICAN:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MEXICAN);
    case vtkKWLanguage::SPANISH_MODERN:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN);
    case vtkKWLanguage::SPANISH_NICARAGUA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_NICARAGUA);
    case vtkKWLanguage::SPANISH_PANAMA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_PANAMA);
    case vtkKWLanguage::SPANISH_PARAGUAY:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_PARAGUAY);
    case vtkKWLanguage::SPANISH_PERU:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_PERU);
    case vtkKWLanguage::SPANISH_PUERTO_RICO:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_PUERTO_RICO);
    case vtkKWLanguage::SPANISH_URUGUAY:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_URUGUAY);
    case vtkKWLanguage::SPANISH_VENEZUELA:
      return (int)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_VENEZUELA);
    case vtkKWLanguage::SWAHILI:
      return (int)MAKELANGID(LANG_SWAHILI, SUBLANG_DEFAULT);
    case vtkKWLanguage::SWEDISH:
      return (int)MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH);
    case vtkKWLanguage::SWEDISH_FINLAND:
      return (int)MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH_FINLAND);
    case vtkKWLanguage::TAMIL:
      return (int)MAKELANGID(LANG_TAMIL, SUBLANG_DEFAULT);
    case vtkKWLanguage::TATAR:
      return (int)MAKELANGID(LANG_TATAR, SUBLANG_DEFAULT);
    case vtkKWLanguage::TELUGU:
      return (int)MAKELANGID(LANG_TELUGU, SUBLANG_DEFAULT);
    case vtkKWLanguage::THAI:
      return (int)MAKELANGID(LANG_THAI, SUBLANG_DEFAULT);
    case vtkKWLanguage::TURKISH:
      return (int)MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT);
    case vtkKWLanguage::UKRAINIAN:
      return (int)MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT);
    case vtkKWLanguage::URDU:
      return (int)MAKELANGID(LANG_URDU, SUBLANG_DEFAULT);
    case vtkKWLanguage::URDU_INDIA:
      return (int)MAKELANGID(LANG_URDU, SUBLANG_URDU_INDIA);
    case vtkKWLanguage::URDU_PAKISTAN:
      return (int)MAKELANGID(LANG_URDU, SUBLANG_URDU_PAKISTAN);
    case vtkKWLanguage::UZBEK:
      return (int)MAKELANGID(LANG_UZBEK, SUBLANG_DEFAULT);
    case vtkKWLanguage::UZBEK_CYRILLIC:
      return (int)MAKELANGID(LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC);
    case vtkKWLanguage::UZBEK_LATIN:
      return (int)MAKELANGID(LANG_UZBEK, SUBLANG_UZBEK_LATIN);
    case vtkKWLanguage::VIETNAMESE:
      return (int)MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT);

    default:
      return (int)MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    }
#else
  (void)lang;
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWLanguage::GetLanguageFromWin32LANGID(int win32langid)
{
#if _WIN32
  WORD primary = PRIMARYLANGID(win32langid);
  WORD sub = SUBLANGID(win32langid);

  if (primary == LANG_AFRIKAANS && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::AFRIKAANS;
  if (primary == LANG_ALBANIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ALBANIAN;
  if (primary == LANG_ARABIC && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ARABIC;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_ALGERIA)
    return vtkKWLanguage::ARABIC_ALGERIA;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_BAHRAIN)
    return vtkKWLanguage::ARABIC_BAHRAIN;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_EGYPT)
    return vtkKWLanguage::ARABIC_EGYPT;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_IRAQ)
    return vtkKWLanguage::ARABIC_IRAQ;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_JORDAN)
    return vtkKWLanguage::ARABIC_JORDAN;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_KUWAIT)
    return vtkKWLanguage::ARABIC_KUWAIT;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_LEBANON)
    return vtkKWLanguage::ARABIC_LEBANON;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_LIBYA)
    return vtkKWLanguage::ARABIC_LIBYA;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_MOROCCO)
    return vtkKWLanguage::ARABIC_MOROCCO;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_OMAN)
    return vtkKWLanguage::ARABIC_OMAN;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_QATAR)
    return vtkKWLanguage::ARABIC_QATAR;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_SAUDI_ARABIA)
    return vtkKWLanguage::ARABIC_SAUDI_ARABIA;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_SYRIA)
    return vtkKWLanguage::ARABIC_SYRIA;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_TUNISIA)
    return vtkKWLanguage::ARABIC_TUNISIA;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_UAE)
    return vtkKWLanguage::ARABIC_UAE;
  if (primary == LANG_ARABIC && sub == SUBLANG_ARABIC_YEMEN)
    return vtkKWLanguage::ARABIC_YEMEN;
  if (primary == LANG_ARMENIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ARMENIAN;
  if (primary == LANG_ASSAMESE && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ASSAMESE;
  if (primary == LANG_AZERI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::AZERI;
  if (primary == LANG_AZERI && sub == SUBLANG_AZERI_CYRILLIC)
    return vtkKWLanguage::AZERI_CYRILLIC;
  if (primary == LANG_AZERI && sub == SUBLANG_AZERI_LATIN)
    return vtkKWLanguage::AZERI_LATIN;
  if (primary == LANG_BASQUE && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::BASQUE;
  if (primary == LANG_BELARUSIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::BELARUSIAN;
  if (primary == LANG_BENGALI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::BENGALI;
  if (primary == LANG_BULGARIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::BULGARIAN;
  if (primary == LANG_CATALAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::CATALAN;
  if (primary == LANG_CHINESE && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::CHINESE;
  if (primary == LANG_CHINESE && sub == SUBLANG_CHINESE_SIMPLIFIED)
    return vtkKWLanguage::CHINESE_SIMPLIFIED;
  if (primary == LANG_CHINESE && sub == SUBLANG_CHINESE_TRADITIONAL)
    return vtkKWLanguage::CHINESE_TRADITIONAL;
  if (primary == LANG_CHINESE && sub == SUBLANG_CHINESE_HONGKONG)
    return vtkKWLanguage::CHINESE_HONGKONG;
  if (primary == LANG_CHINESE && sub == SUBLANG_CHINESE_MACAU)
    return vtkKWLanguage::CHINESE_MACAU;
  if (primary == LANG_CHINESE && sub == SUBLANG_CHINESE_SINGAPORE)
    return vtkKWLanguage::CHINESE_SINGAPORE;
  if (primary == LANG_CHINESE && sub == SUBLANG_CHINESE_TRADITIONAL)
    return vtkKWLanguage::CHINESE_TAIWAN;
  if (primary == LANG_CROATIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::CROATIAN;
  if (primary == LANG_CZECH && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::CZECH;
  if (primary == LANG_DANISH && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::DANISH;
  if (primary == LANG_DUTCH && sub == SUBLANG_DUTCH)
    return vtkKWLanguage::DUTCH;
  if (primary == LANG_DUTCH && sub == SUBLANG_DUTCH_BELGIAN)
    return vtkKWLanguage::DUTCH_BELGIAN;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_UK)
    return vtkKWLanguage::ENGLISH;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_UK)
    return vtkKWLanguage::ENGLISH_UK;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_US)
    return vtkKWLanguage::ENGLISH_US;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_AUS)
    return vtkKWLanguage::ENGLISH_AUSTRALIA;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_BELIZE)
    return vtkKWLanguage::ENGLISH_BELIZE;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_CAN)
    return vtkKWLanguage::ENGLISH_CANADA;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_CARIBBEAN)
    return vtkKWLanguage::ENGLISH_CARIBBEAN;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_EIRE)
    return vtkKWLanguage::ENGLISH_EIRE;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_JAMAICA)
    return vtkKWLanguage::ENGLISH_JAMAICA;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_NZ)
    return vtkKWLanguage::ENGLISH_NEW_ZEALAND;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_PHILIPPINES)
    return vtkKWLanguage::ENGLISH_PHILIPPINES;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_SOUTH_AFRICA)
    return vtkKWLanguage::ENGLISH_SOUTH_AFRICA;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_TRINIDAD)
    return vtkKWLanguage::ENGLISH_TRINIDAD;
  if (primary == LANG_ENGLISH && sub == SUBLANG_ENGLISH_ZIMBABWE)
    return vtkKWLanguage::ENGLISH_ZIMBABWE;
  if (primary == LANG_ESTONIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ESTONIAN;
  if (primary == LANG_FAEROESE && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::FAEROESE;
  if (primary == LANG_FARSI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::FARSI;
  if (primary == LANG_FINNISH && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::FINNISH;
  if (primary == LANG_FRENCH && sub == SUBLANG_FRENCH)
    return vtkKWLanguage::FRENCH;
  if (primary == LANG_FRENCH && sub == SUBLANG_FRENCH_BELGIAN)
    return vtkKWLanguage::FRENCH_BELGIAN;
  if (primary == LANG_FRENCH && sub == SUBLANG_FRENCH_CANADIAN)
    return vtkKWLanguage::FRENCH_CANADIAN;
  if (primary == LANG_FRENCH && sub == SUBLANG_FRENCH_LUXEMBOURG)
    return vtkKWLanguage::FRENCH_LUXEMBOURG;
  if (primary == LANG_FRENCH && sub == SUBLANG_FRENCH_MONACO)
    return vtkKWLanguage::FRENCH_MONACO;
  if (primary == LANG_FRENCH && sub == SUBLANG_FRENCH_SWISS)
    return vtkKWLanguage::FRENCH_SWISS;
  if (primary == LANG_GEORGIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::GEORGIAN;
  if (primary == LANG_GERMAN && sub == SUBLANG_GERMAN)
    return vtkKWLanguage::GERMAN;
  if (primary == LANG_GERMAN && sub == SUBLANG_GERMAN_AUSTRIAN)
    return vtkKWLanguage::GERMAN_AUSTRIAN;
  if (primary == LANG_GERMAN && sub == SUBLANG_GERMAN_LIECHTENSTEIN)
    return vtkKWLanguage::GERMAN_LIECHTENSTEIN;
  if (primary == LANG_GERMAN && sub == SUBLANG_GERMAN_LUXEMBOURG)
    return vtkKWLanguage::GERMAN_LUXEMBOURG;
  if (primary == LANG_GERMAN && sub == SUBLANG_GERMAN_SWISS)
    return vtkKWLanguage::GERMAN_SWISS;
  if (primary == LANG_GREEK && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::GREEK;
  if (primary == LANG_GUJARATI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::GUJARATI;
  if (primary == LANG_HEBREW && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::HEBREW;
  if (primary == LANG_HINDI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::HINDI;
  if (primary == LANG_HUNGARIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::HUNGARIAN;
  if (primary == LANG_ICELANDIC && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ICELANDIC;
  if (primary == LANG_INDONESIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::INDONESIAN;
  if (primary == LANG_ITALIAN && sub == SUBLANG_ITALIAN)
    return vtkKWLanguage::ITALIAN;
  if (primary == LANG_ITALIAN && sub == SUBLANG_ITALIAN_SWISS)
    return vtkKWLanguage::ITALIAN_SWISS;
  if (primary == LANG_JAPANESE && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::JAPANESE;
  if (primary == LANG_KANNADA && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::KANNADA;
  if (primary == LANG_KASHMIRI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::KASHMIRI;
  if (primary == LANG_KASHMIRI && sub == SUBLANG_KASHMIRI_INDIA)
    return vtkKWLanguage::KASHMIRI_INDIA;
  if (primary == LANG_KAZAK && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::KAZAKH;
  if (primary == LANG_KONKANI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::KONKANI;
  if (primary == LANG_KOREAN && sub == SUBLANG_KOREAN)
    return vtkKWLanguage::KOREAN;
  if (primary == LANG_LATVIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::LATVIAN;
  if (primary == LANG_LITHUANIAN && sub == SUBLANG_LITHUANIAN)
    return vtkKWLanguage::LITHUANIAN;
  if (primary == LANG_MACEDONIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::MACEDONIAN;
  if (primary == LANG_MALAY && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::MALAY;
  if (primary == LANG_MALAYALAM && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::MALAYALAM;
  if (primary == LANG_MALAY && sub == SUBLANG_MALAY_BRUNEI_DARUSSALAM)
    return vtkKWLanguage::MALAY_BRUNEI_DARUSSALAM;
  if (primary == LANG_MALAY && sub == SUBLANG_MALAY_MALAYSIA)
    return vtkKWLanguage::MALAY_MALAYSIA;
  if (primary == LANG_MANIPURI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::MANIPURI;
  if (primary == LANG_MARATHI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::MARATHI;
  if (primary == LANG_NEPALI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::NEPALI;
  if (primary == LANG_NEPALI && sub == SUBLANG_NEPALI_INDIA)
    return vtkKWLanguage::NEPALI_INDIA;
  if (primary == LANG_NORWEGIAN && sub == SUBLANG_NORWEGIAN_BOKMAL)
    return vtkKWLanguage::NORWEGIAN_BOKMAL;
  if (primary == LANG_NORWEGIAN && sub == SUBLANG_NORWEGIAN_NYNORSK)
    return vtkKWLanguage::NORWEGIAN_NYNORSK;
  if (primary == LANG_ORIYA && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ORIYA;
  if (primary == LANG_POLISH && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::POLISH;
  if (primary == LANG_PORTUGUESE && sub == SUBLANG_PORTUGUESE)
    return vtkKWLanguage::PORTUGUESE;
  if (primary == LANG_PORTUGUESE && sub == SUBLANG_PORTUGUESE_BRAZILIAN)
    return vtkKWLanguage::PORTUGUESE_BRAZILIAN;
  if (primary == LANG_PUNJABI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::PUNJABI;
  if (primary == LANG_ROMANIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::ROMANIAN;
  if (primary == LANG_RUSSIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::RUSSIAN;
  if (primary == LANG_SANSKRIT && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::SANSKRIT;
  if (primary == LANG_SERBIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::SERBIAN;
  if (primary == LANG_SERBIAN && sub == SUBLANG_SERBIAN_CYRILLIC)
    return vtkKWLanguage::SERBIAN_CYRILLIC;
  if (primary == LANG_SERBIAN && sub == SUBLANG_SERBIAN_LATIN)
    return vtkKWLanguage::SERBIAN_LATIN;
  if (primary == LANG_SINDHI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::SINDHI;
  if (primary == LANG_SLOVAK && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::SLOVAK;
  if (primary == LANG_SLOVENIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::SLOVENIAN;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH)
    return vtkKWLanguage::SPANISH;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_ARGENTINA)
    return vtkKWLanguage::SPANISH_ARGENTINA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_BOLIVIA)
    return vtkKWLanguage::SPANISH_BOLIVIA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_CHILE)
    return vtkKWLanguage::SPANISH_CHILE;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_COLOMBIA)
    return vtkKWLanguage::SPANISH_COLOMBIA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_COSTA_RICA)
    return vtkKWLanguage::SPANISH_COSTA_RICA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_DOMINICAN_REPUBLIC)
    return vtkKWLanguage::SPANISH_DOMINICAN_REPUBLIC;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_ECUADOR)
    return vtkKWLanguage::SPANISH_ECUADOR;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_EL_SALVADOR)
    return vtkKWLanguage::SPANISH_EL_SALVADOR;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_GUATEMALA)
    return vtkKWLanguage::SPANISH_GUATEMALA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_HONDURAS)
    return vtkKWLanguage::SPANISH_HONDURAS;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_MEXICAN)
    return vtkKWLanguage::SPANISH_MEXICAN;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_MODERN)
    return vtkKWLanguage::SPANISH_MODERN;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_NICARAGUA)
    return vtkKWLanguage::SPANISH_NICARAGUA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_PANAMA)
    return vtkKWLanguage::SPANISH_PANAMA;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_PARAGUAY)
    return vtkKWLanguage::SPANISH_PARAGUAY;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_PERU)
    return vtkKWLanguage::SPANISH_PERU;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_PUERTO_RICO)
    return vtkKWLanguage::SPANISH_PUERTO_RICO;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_URUGUAY)
    return vtkKWLanguage::SPANISH_URUGUAY;
  if (primary == LANG_SPANISH && sub == SUBLANG_SPANISH_VENEZUELA)
    return vtkKWLanguage::SPANISH_VENEZUELA;
  if (primary == LANG_SWAHILI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::SWAHILI;
  if (primary == LANG_SWEDISH && sub == SUBLANG_SWEDISH)
    return vtkKWLanguage::SWEDISH;
  if (primary == LANG_SWEDISH && sub == SUBLANG_SWEDISH_FINLAND)
    return vtkKWLanguage::SWEDISH_FINLAND;
  if (primary == LANG_TAMIL && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::TAMIL;
  if (primary == LANG_TATAR && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::TATAR;
  if (primary == LANG_TELUGU && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::TELUGU;
  if (primary == LANG_THAI && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::THAI;
  if (primary == LANG_TURKISH && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::TURKISH;
  if (primary == LANG_UKRAINIAN && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::UKRAINIAN;
  if (primary == LANG_URDU && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::URDU;
  if (primary == LANG_URDU && sub == SUBLANG_URDU_INDIA)
    return vtkKWLanguage::URDU_INDIA;
  if (primary == LANG_URDU && sub == SUBLANG_URDU_PAKISTAN)
    return vtkKWLanguage::URDU_PAKISTAN;
  if (primary == LANG_UZBEK && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::UZBEK;
  if (primary == LANG_UZBEK && sub == SUBLANG_UZBEK_CYRILLIC)
    return vtkKWLanguage::UZBEK_CYRILLIC;
  if (primary == LANG_UZBEK && sub == SUBLANG_UZBEK_LATIN)
    return vtkKWLanguage::UZBEK_LATIN;
  if (primary == LANG_VIETNAMESE && sub == SUBLANG_DEFAULT)
    return vtkKWLanguage::VIETNAMESE;
#else  
  (void)win32langid;
#endif
  return vtkKWLanguage::UNKNOWN;
}

//----------------------------------------------------------------------------
void vtkKWLanguage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
