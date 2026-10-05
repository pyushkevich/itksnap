/**
 * TestTranslationLocale.cxx
 *
 * Verifies how ITK-SNAP chooses the language of its user interface from the
 * preferred UI languages of the user (#210). The translations available in
 * this test match those shipped with ITK-SNAP: de, es and zh_CN.
 */

#include "QtTranslationLocale.h"

#include <cstdlib>
#include <iostream>

static int errors = 0;

static bool
hasTranslation(const QLocale &locale)
{
  return locale.language() == QLocale::German || locale.language() == QLocale::Spanish ||
         locale.name() == "zh_CN";
}

static void
checkSelection(const QStringList &preferred, const QString &expectedName)
{
  QString selected = SelectTranslationLocale(preferred, hasTranslation).name();
  QString list = preferred.join(",");
  if (selected != expectedName)
  {
    std::cerr << "FAILED: [" << list.toStdString() << "] selected " << selected.toStdString()
              << ", expected " << expectedName.toStdString() << std::endl;
    errors++;
  }
  else
  {
    std::cout << "[" << list.toStdString() << "] -> " << selected.toStdString() << std::endl;
  }
}

static void
checkPOSIX(const QString     &language,
           const QString     &lc_all,
           const QString     &lc_messages,
           const QString     &lang,
           const QStringList &expected)
{
  QStringList result = ParsePOSIXLanguageVariables(language, lc_all, lc_messages, lang);
  if (result != expected)
  {
    std::cerr << "FAILED: POSIX variables LANGUAGE=" << language.toStdString()
              << " LC_ALL=" << lc_all.toStdString() << " LC_MESSAGES=" << lc_messages.toStdString()
              << " LANG=" << lang.toStdString() << " gave [" << result.join(",").toStdString()
              << "], expected [" << expected.join(",").toStdString() << "]" << std::endl;
    errors++;
  }
}

int
main()
{
  // English speakers keep English, whatever else is in their list
  checkSelection({ "en-US" }, "en_US");
  checkSelection({ "en-GB", "de-DE" }, "en_GB");
  checkSelection({ "en-IN", "de-DE" }, "en_IN");
  checkSelection({ "en-DE", "de-DE" }, "en_DE");

  // A translated language is used when it is preferred
  checkSelection({ "de-DE", "en-US" }, "de_DE");
  checkSelection({ "es-ES" }, "es_ES");
  checkSelection({ "es-419", "en-US" }, "es_419");

  // Languages without a translation are skipped in favor of the next one
  checkSelection({ "fr-FR", "de-DE" }, "de_DE");
  checkSelection({ "fr-FR" }, "en_US");
  checkSelection({ "tr-TR", "fr-FR" }, "en_US");

  // Simplified Chinese from any territory uses the zh_CN translation,
  // Traditional Chinese has no translation
  checkSelection({ "zh-Hans-US" }, "zh_CN");
  checkSelection({ "zh-CN" }, "zh_CN");
  checkSelection({ "zh-Hant-TW", "es-ES" }, "es_ES");

  // Nothing usable falls back to English
  checkSelection({}, "en_US");
  checkSelection({ "C" }, "en_US");

  // POSIX environment: LANGUAGE list first, then LC_ALL, LC_MESSAGES, LANG
  checkPOSIX("", "", "", "de_DE.UTF-8", { "de_DE" });
  checkPOSIX("", "", "en_US.UTF-8", "de_DE.UTF-8", { "en_US" });
  checkPOSIX("", "es_ES.UTF-8", "en_US.UTF-8", "de_DE.UTF-8", { "es_ES" });
  checkPOSIX("en_GB:de", "", "", "de_DE.UTF-8", { "en_GB", "de" });
  checkPOSIX("", "", "", "de_DE@euro", { "de_DE" });
  checkPOSIX("", "", "", "", {});

  // English user with a German LANG but English messages gets English
  checkSelection(ParsePOSIXLanguageVariables("", "", "en_US.UTF-8", "de_DE.UTF-8"), "en_US");

  if (errors == 0)
    std::cout << "All translation locale tests passed" << std::endl;
  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
