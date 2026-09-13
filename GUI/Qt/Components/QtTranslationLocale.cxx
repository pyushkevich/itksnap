#include "QtTranslationLocale.h"

#if defined(__APPLE__)
#  include <CoreFoundation/CoreFoundation.h>
#elif defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <vector>
#else
#  include <QtGlobal>
#endif

QStringList
ParsePOSIXLanguageVariables(const QString &language,
                            const QString &lc_all,
                            const QString &lc_messages,
                            const QString &lang)
{
  QStringList entries;
  if (!language.isEmpty())
    entries = language.split(':', Qt::SkipEmptyParts);
  else if (!lc_all.isEmpty())
    entries << lc_all;
  else if (!lc_messages.isEmpty())
    entries << lc_messages;
  else if (!lang.isEmpty())
    entries << lang;

  // Remove codeset and modifier, as in de_DE.UTF-8 or de_DE@euro
  QStringList result;
  for (QString entry : entries)
  {
    for (QChar c : { QChar('.'), QChar('@') })
      if (entry.contains(c))
        entry.truncate(entry.indexOf(c));
    if (!entry.isEmpty())
      result << entry;
  }
  return result;
}

QStringList
GetPreferredUILanguages()
{
  QStringList result;

#if defined(__APPLE__)
  // This honors the per-application language set in System Settings
  CFArrayRef languages = CFLocaleCopyPreferredLanguages();
  if (languages)
  {
    for (CFIndex i = 0; i < CFArrayGetCount(languages); i++)
      result << QString::fromCFString(static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages, i)));
    CFRelease(languages);
  }
#elif defined(_WIN32)
  ULONG count = 0, size = 0;
  if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, nullptr, &size) && size > 0)
  {
    std::vector<wchar_t> buffer(size);
    if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer.data(), &size))
    {
      // The buffer holds null-terminated names followed by an empty string
      for (const wchar_t *p = buffer.data(); *p; p += wcslen(p) + 1)
        result << QString::fromWCharArray(p);
    }
  }
#else
  result = ParsePOSIXLanguageVariables(qEnvironmentVariable("LANGUAGE"),
                                       qEnvironmentVariable("LC_ALL"),
                                       qEnvironmentVariable("LC_MESSAGES"),
                                       qEnvironmentVariable("LANG"));
#endif

  return result;
}

QLocale
SelectTranslationLocale(const QStringList                          &preferred,
                        const std::function<bool(const QLocale &)> &hasTranslation)
{
  for (const QString &entry : preferred)
  {
    QLocale locale(entry);
    if (locale.language() == QLocale::C || locale.language() == QLocale::AnyLanguage)
      continue;

    // English is the source language and needs no translation
    if (locale.language() == QLocale::English)
      return locale;

    // Simplified Chinese is reported with other territories on MacOS (e.g.,
    // zh-Hans-US), but its translation is named for zh_CN
    if (locale.language() == QLocale::Chinese && locale.script() == QLocale::SimplifiedChineseScript)
      locale = QLocale(QLocale::Chinese, QLocale::China);

    if (hasTranslation(locale))
      return locale;
  }

  return QLocale(QLocale::English, QLocale::UnitedStates);
}
