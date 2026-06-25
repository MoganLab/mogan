
/******************************************************************************
 * MODULE     : locale.cpp
 * DESCRIPTION: Locale related routines
 * COPYRIGHT  : (C) 1999-2019  Joris van der Hoeven, Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "locale.hpp"

#include <clocale>
#include <cstring>
#include <ctime>

#if !defined(OS_MINGW) && !defined(OS_WIN)
#include <langinfo.h>
#ifndef X11TEXMACS
#include <locale>
#endif
#endif

#if defined(OS_WIN)
#define NOGDI
#include <windows.h>
#include <winnls.h>
#elif defined(OS_MINGW)
#include <winnls.h>
#endif

#define outline Core_outline
#define extend Core_extends
#ifdef OS_MACOS
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFString.h>
#endif
#undef extend
#undef outline

#include "analyze.hpp"
#include "sys_utils.hpp"

/******************************************************************************
 * Locales
 ******************************************************************************/

#if defined(OS_MINGW) || defined(OS_WIN)
const string
windows_locale_to_language () {
  static string language;

  if (N (language) == 0) {
    LANGID lid= GetUserDefaultUILanguage ();
    switch (PRIMARYLANGID (lid)) {
    case LANG_BULGARIAN:
      language= "bulgarian";
      break;
    case LANG_CHINESE:
      language= "chinese";
      break;
    case LANG_CHINESE_TRADITIONAL:
      language= "taiwanese";
      break;
    case LANG_CROATIAN:
      language= "croatian";
      break;
    case LANG_CZECH:
      language= "czech";
      break;
    case LANG_DANISH:
      language= "danish";
      break;
    case LANG_DUTCH:
      language= "dutch";
      break;
    case LANG_ENGLISH:
      switch (SUBLANGID (lid)) {
      case SUBLANG_ENGLISH_UK:
        language= "british";
        break;
      default:
        language= "english";
        break;
      }
      break;
    case LANG_FRENCH:
      language= "french";
      break;
    case LANG_GERMAN:
      language= "german";
      break;
    case LANG_GREEK:
      language= "greek";
      break;
    case LANG_HUNGARIAN:
      language= "hungarian";
      break;
    case LANG_ITALIAN:
      language= "italian";
      break;
    case LANG_JAPANESE:
      language= "japanese";
      break;
    case LANG_KOREAN:
      language= "korean";
      break;
    case LANG_POLISH:
      language= "polish";
      break;
    case LANG_PORTUGUESE:
      language= "portuguese";
      break;
    case LANG_ROMANIAN:
      language= "romanian";
      break;
    case LANG_RUSSIAN:
      language= "russian";
      break;
    case LANG_SLOVAK:
      language= "slovak";
      break;
    case LANG_SLOVENIAN:
      language= "slovene";
      break;
    case LANG_SPANISH:
      language= "spanish";
      break;
    case LANG_SWEDISH:
      language= "swedish";
      break;
    case LANG_UKRAINIAN:
      language= "ukrainian";
      break;
    default:
      language= "english";
      break;
    }
  }
  return language;
}
#endif

#ifdef OS_MACOS
string
get_mac_language () {
  char        mac_lang[50];
  CFLocaleRef locale= CFLocaleCopyCurrent ();
  CFTypeRef   lang  = CFLocaleGetValue (locale, kCFLocaleLanguageCode);
  CFStringGetCString ((CFStringRef) lang, mac_lang, sizeof (mac_lang),
                      kCFStringEncodingUTF8);
  CFRelease (locale);
  return string (mac_lang);
}
#endif

string
locale_to_language (string s) {
  if (N (s) > 5) s= s (0, 5);
  if (s == "en_GB") return "british";
  if (s == "zh_TW") return "taiwanese";
  if (N (s) > 2) s= s (0, 2);
  if (s == "bg") return "bulgarian";
  if (s == "zh") return "chinese";
  if (s == "hr") return "croatian";
  if (s == "cs") return "czech";
  if (s == "da") return "danish";
  if (s == "nl") return "dutch";
  if (s == "en") return "english";
  if (s == "eo") return "esperanto";
  if (s == "fi") return "finnish";
  if (s == "fr") return "french";
  if (s == "de") return "german";
  if (s == "gr") return "greek";
  if (s == "hu") return "hungarian";
  if (s == "it") return "italian";
  if (s == "ja") return "japanese";
  if (s == "ko") return "korean";
  if (s == "pl") return "polish";
  if (s == "pt") return "portuguese";
  if (s == "ro") return "romanian";
  if (s == "ru") return "russian";
  if (s == "sk") return "slovak";
  if (s == "sl") return "slovene";
  if (s == "es") return "spanish";
  if (s == "sv") return "swedish";
  if (s == "uk") return "ukrainian";
  return "english";
}

string
language_to_locale (string s) {
  if (s == "american") return "en_US";
  if (s == "british") return "en_GB";
  if (s == "bulgarian") return "bg_BG";
  if (s == "chinese") return "zh_CN";
  if (s == "croatian") return "hr_HR";
  if (s == "czech") return "cs_CZ";
  if (s == "danish") return "da_DK";
  if (s == "dutch") return "nl_NL";
  if (s == "english") return "en_US";
  if (s == "esperanto") return "eo_EO";
  if (s == "finnish") return "fi_FI";
  if (s == "french") return "fr_FR";
  if (s == "german") return "de_DE";
  if (s == "greek") return "gr_GR";
  if (s == "hungarian") return "hu_HU";
  if (s == "italian") return "it_IT";
  if (s == "japanese") return "ja_JP";
  if (s == "korean") return "ko_KR";
  if (s == "polish") return "pl_PL";
  if (s == "portuguese") return "pt_PT";
  if (s == "romanian") return "ro_RO";
  if (s == "russian") return "ru_RU";
  if (s == "slovak") return "sk_SK";
  if (s == "slovene") return "sl_SI";
  if (s == "spanish") return "es_ES";
  if (s == "swedish") return "sv_SV";
  if (s == "taiwanese") return "zh_TW";
  if (s == "ukrainian") return "uk_UA";
  return "en_US";
}

string
language_to_local_ISO_charset (string s) {
  if (s == "bulgarian") return "ISO-8859-5";
  if (s == "chinese") return "";
  if (s == "croatian") return "ISO-8859-2";
  if (s == "czech") return "ISO-8859-2";
  if (s == "greek") return "ISO-8859-7";
  if (s == "hungarian") return "ISO-8859-2";
  if (s == "japanese") return "";
  if (s == "korean") return "";
  if (s == "polish") return "ISO-8859-2";
  if (s == "romanian") return "ISO-8859-2";
  if (s == "russian") return "ISO-8859-5";
  if (s == "slovak") return "ISO-8859-2";
  if (s == "slovene") return "ISO-8859-2";
  if (s == "taiwanese") return "";
  if (s == "ukrainian") return "ISO-8859-5";
  return "ISO-8859-1";
}

string
get_locale_language () {
#if defined(OS_MINGW) || defined(OS_WIN)
  return windows_locale_to_language ();
#endif

#ifdef OS_MACOS
  return locale_to_language (get_mac_language ());
#endif

#ifdef OS_WASM
  return "english";
#endif

#ifdef OS_LINUX
  string env_lan= get_env ("LC_ALL");
  if (env_lan != "") return locale_to_language (env_lan);
  env_lan= get_env ("LC_MESSAGES");
  if (env_lan != "") return locale_to_language (env_lan);
  env_lan= get_env ("LANG");
  if (env_lan != "") return locale_to_language (env_lan);
  env_lan= get_env ("GDM_LANG");
  if (env_lan != "") return locale_to_language (env_lan);
#endif
  return "english";
}

string
get_locale_charset () {
  return "UTF-8";
}

namespace lolly {
namespace locale {

/******************************************************************************
 * Date helpers
 ******************************************************************************/

static int
days_in_month (int year, int month) {
  static int days_per_month[12]= {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
  if (month == 2) {
    bool leap= (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }
  return days_per_month[month - 1];
}

static int
weekday (int year, int month, int day) {
  struct tm tm;
  memset (&tm, 0, sizeof (tm));
  tm.tm_year = year - 1900;
  tm.tm_mon  = month - 1;
  tm.tm_mday = day;
  tm.tm_isdst= -1;
  mktime (&tm);
  return tm.tm_wday;
}

/******************************************************************************
 * Month and weekday names
 ******************************************************************************/

static const char* en_months_full[12]= {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December"};
static const char* en_months_abbr[12]= {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char* en_weekdays_full[7]= {
    "Sunday",   "Monday",   "Tuesday", "Wednesday", "Thursday",
    "Friday",   "Saturday"};
static const char* en_weekdays_abbr[7]= {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static const char* de_months_full[12]= {
    "Januar",  "Februar", "M<#e4>rz", "April",    "Mai",       "Juni",
    "Juli",    "August",  "September", "Oktober", "November", "Dezember"};
static const char* de_months_abbr[12]= {
    "Jan", "Feb", "M<#e4>r", "Apr", "Mai", "Jun",
    "Jul", "Aug", "Sep",     "Okt", "Nov", "Dez"};
static const char* de_weekdays_full[7]= {
    "Sonntag",   "Montag",   "Dienstag", "Mittwoch",
    "Donnerstag", "Freitag", "Samstag"};
static const char* de_weekdays_abbr[7]= {
    "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

static string
month_name (string lan, int month, bool abbrev) {
  int idx= month - 1;
  if (lan == "german") {
    return abbrev ? de_months_abbr[idx] : de_months_full[idx];
  }
  return abbrev ? en_months_abbr[idx] : en_months_full[idx];
}

static string
weekday_name (string lan, int wd, bool abbrev) {
  if (lan == "german") {
    return abbrev ? de_weekdays_abbr[wd] : de_weekdays_full[wd];
  }
  return abbrev ? en_weekdays_abbr[wd] : en_weekdays_full[wd];
}

/******************************************************************************
 * Qt-style date format
 ******************************************************************************/

static string
format_qt (string lan, string fm, int year, int month, int day) {
  string r;
  int    i= 0, n= N (fm);
  while (i < n) {
    char c= fm[i];
    if (c == 'y') {
      int j= i;
      while (j < n && fm[j] == 'y')
        j++;
      int len= j - i;
      if (len >= 4)
        r << as_string (year);
      else if (len == 2) {
        int y= year % 100;
        if (y < 10) r << '0';
        r << as_string (y);
      }
      i= j;
    }
    else if (c == 'M') {
      int j= i;
      while (j < n && fm[j] == 'M')
        j++;
      int len= j - i;
      if (len >= 4)
        r << month_name (lan, month, false);
      else if (len == 3)
        r << month_name (lan, month, true);
      else if (len == 2) {
        if (month < 10) r << '0';
        r << as_string (month);
      }
      else
        r << as_string (month);
      i= j;
    }
    else if (c == 'd') {
      int j= i;
      while (j < n && fm[j] == 'd')
        j++;
      int len= j - i;
      if (len >= 4)
        r << weekday_name (lan, weekday (year, month, day), false);
      else if (len == 3)
        r << weekday_name (lan, weekday (year, month, day), true);
      else if (len == 2) {
        if (day < 10) r << '0';
        r << as_string (day);
      }
      else
        r << as_string (day);
      i= j;
    }
    else {
      r << c;
      i++;
    }
  }
  return r;
}

/******************************************************************************
 * strftime date format
 ******************************************************************************/

static string
format_strftime (string lan, string fm, int year, int month, int day) {
  struct tm tm;
  memset (&tm, 0, sizeof (tm));
  tm.tm_year = year - 1900;
  tm.tm_mon  = month - 1;
  tm.tm_mday = day;
  tm.tm_isdst= -1;
  mktime (&tm);

  char old_locale_buf[64];
  const char* old_locale= setlocale (LC_TIME, nullptr);
  int         old_len    = strlen (old_locale);
  for (int i= 0; i < old_len; i++)
    old_locale_buf[i]= old_locale[i];
  old_locale_buf[old_len]= '\0';

  char locale_buf[64];
  string locale_s= language_to_locale (lan);
  int    locale_len= N (locale_s);
  for (int i= 0; i < locale_len; i++)
    locale_buf[i]= locale_s[i];
  locale_buf[locale_len]= '\0';
  setlocale (LC_TIME, locale_buf);

  char fm_buf[64];
  int  fm_len= N (fm);
  for (int i= 0; i < fm_len; i++)
    fm_buf[i]= fm[i];
  fm_buf[fm_len]= '\0';

  char buf[256];
  strftime (buf, sizeof (buf), fm_buf, &tm);

  setlocale (LC_TIME, old_locale_buf);
  return string (buf);
}

/******************************************************************************
 * Getting a formatted date
 ******************************************************************************/

string
get_date (string lan, string fm, int year, int month, int day) {
  if (year == -1 && month == -1 && day == -1) {
    time_t     now  = time (nullptr);
    struct tm* local= localtime (&now);
    year            = local->tm_year + 1900;
    month           = local->tm_mon + 1;
    day             = local->tm_mday;
  }

  if (fm == "") {
    if (lan == "british" || lan == "english" || lan == "american")
      return format_qt (lan, "MMMM d, yyyy", year, month, day);
    else if (lan == "german")
      return format_qt (lan, "d. MMMM yyyy", year, month, day);
    else if (lan == "chinese" || lan == "japanese" || lan == "taiwanese")
      return as_string (year) * "<#5e74>" * as_string (month) * "<#6708>" *
             as_string (day) * "<#65e5>";
    else if (lan == "korean")
      return as_string (year) * "<#b144> " * as_string (month) * "<#c6d4> " *
             as_string (day) * "<#c77c>";
    else
      return format_qt (lan, "d MMMM yyyy", year, month, day);
  }

  if (N (fm) > 0 && fm[0] == '%')
    return format_strftime (lan, fm, year, month, day);

  return format_qt (lan, fm, year, month, day);
}

} // namespace locale
} // namespace lolly
