#ifndef QTTRANSLATIONLOCALE_H
#define QTTRANSLATIONLOCALE_H

#include <QLocale>
#include <QString>
#include <QStringList>
#include <functional>

/**
 * The user's preferred user interface languages, most preferred first, as
 * reported by the operating system (e.g., "en-GB", "de-DE").
 *
 * QLocale::system().uiLanguages() is not suitable for choosing a translation:
 * when the regional format locale (e.g., de_DE for a user in Germany) is not
 * one of the preferred languages, Qt puts it first in the list, so the user
 * gets a language they did not choose (see QTBUG-104930 in qlocale.cpp).
 */
QStringList GetPreferredUILanguages();

/**
 * Preferred UI languages from the POSIX locale environment variables:
 * the colon-separated LANGUAGE list, otherwise the first set of LC_ALL,
 * LC_MESSAGES and LANG. Codeset and modifier suffixes are removed.
 */
QStringList ParsePOSIXLanguageVariables(const QString &language,
                                        const QString &lc_all,
                                        const QString &lc_messages,
                                        const QString &lang);

/**
 * Choose the locale used to load ITK-SNAP's translation: the first preferred
 * language for which hasTranslation() returns true. English, the source
 * language of the user interface, is always available and is also returned
 * when no preferred language has a translation.
 */
QLocale SelectTranslationLocale(const QStringList                          &preferred,
                                const std::function<bool(const QLocale &)> &hasTranslation);

#endif // QTTRANSLATIONLOCALE_H
