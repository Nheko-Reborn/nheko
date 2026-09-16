// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QStandardPaths>

#include "Logging.h"
#include "Theme.h"

namespace {
QDir
themesDir()
{
    // Not AppDataLocation: with both the organization and application name set to "nheko",
    // that resolves to .../nheko/nheko (e.g. ~/.local/share/nheko/nheko on Linux, alongside
    // this app's own database) - a real path, but a confusing one to point users at. Building
    // it from GenericDataLocation instead gives the plain ~/.local/share/nheko/themes.
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                QStringLiteral("/nheko/themes"));
}

// Reads one color key out of a theme JSON object. Clears `ok` (rather than throwing or
// aborting) on anything invalid, so load() can finish scanning all the keys and log a single
// complete failure instead of stopping at the first bad one.
QColor
jsonColor(const QJsonObject &obj, QLatin1String key, bool &ok)
{
    auto v = obj.value(key);
    if (!v.isString()) {
        ok = false;
        return {};
    }

    QColor c(v.toString());
    if (!c.isValid())
        ok = false;

    return c;
}

// nhlog::ui() can still be null here: applyTheme() runs during UserSettings::load() at
// startup (main.cpp), which happens before nhlog::init() sets the loggers up - so a theme
// name that fails to load this early (a typo, or a custom theme file that was since removed)
// would otherwise crash trying to log about it instead of just silently falling back.
template<typename... Args>
void
logWarn(fmt::format_string<Args...> fmt, Args &&...args)
{
    if (auto logger = nhlog::ui())
        logger->warn(fmt, std::forward<Args>(args)...);
}
}

QStringList
ThemeLoader::customThemeIds()
{
    QStringList ids;
    for (const auto &info :
         themesDir().entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name))
        ids << info.completeBaseName();
    return ids;
}

std::optional<CustomTheme>
ThemeLoader::load(const QString &id)
{
    static QMap<QString, std::optional<CustomTheme>> cache;
    if (auto it = cache.find(id); it != cache.end())
        return it.value();

    auto path = themesDir().filePath(id + QStringLiteral(".json"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        logWarn("Custom theme '{}': could not open {}", id.toStdString(), path.toStdString());
        return cache.insert(id, std::nullopt).value();
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        logWarn("Custom theme '{}': invalid JSON ({})",
                id.toStdString(),
                err.errorString().toStdString());
        return cache.insert(id, std::nullopt).value();
    }

    auto obj = doc.object();
    bool ok  = true;
    auto col = [&](const char *key) { return jsonColor(obj, QLatin1String(key), ok); };

    QPalette pal(/*windowText*/ col("windowText"),
                 /*button*/ col("button"),
                 /*light*/ col("light"),
                 /*dark*/ col("dark"),
                 /*mid*/ col("mid"),
                 /*text*/ col("text"),
                 /*bright_text*/ col("brightText"),
                 /*base*/ col("base"),
                 /*window*/ col("window"));
    pal.setColor(QPalette::AlternateBase, col("alternateBase"));
    pal.setColor(QPalette::Highlight, col("highlight"));
    pal.setColor(QPalette::HighlightedText, col("highlightedText"));
    pal.setColor(QPalette::ToolTipBase, col("tooltipBase"));
    pal.setColor(QPalette::ToolTipText, col("tooltipText"));
    pal.setColor(QPalette::Link, col("link"));
    pal.setColor(QPalette::ButtonText, col("buttonText"));

    CustomTheme theme;
    theme.palette           = pal;
    theme.sidebarBackground = col("sidebarBackground");
    theme.alternateButton   = col("alternateButton");
    theme.red               = col("red");
    theme.green             = col("green");
    theme.orange            = col("orange");
    theme.error             = col("error");
    theme.displayName       = obj.value(QStringLiteral("name")).toString(id);

    if (!ok) {
        logWarn("Custom theme '{}': missing or invalid color key in {}",
                id.toStdString(),
                path.toStdString());
        return cache.insert(id, std::nullopt).value();
    }

    return cache.insert(id, theme).value();
}

QString
ThemeLoader::displayName(const QString &id)
{
    if (auto t = load(id))
        return t->displayName;
    return id;
}

QPalette
Theme::paletteFromTheme(QStringView theme)
{
    static QPalette original = QGuiApplication::palette();
    if (theme == u"light") {
        static QPalette lightActive = [] {
            QPalette lightActive(
              /*windowText*/ QColor(0x33, 0x33, 0x33),
              /*button*/ QColor(Qt::GlobalColor::white),
              /*light*/ QColor(0xef, 0xef, 0xef),
              /*dark*/ QColor(70, 77, 93),
              /*mid*/ QColor(220, 220, 220),
              /*text*/ QColor(0x33, 0x33, 0x33),
              /*bright_text*/ QColor(0xf2, 0xf5, 0xf8),
              /*base*/ QColor(Qt::GlobalColor::white),
              /*window*/ QColor(Qt::GlobalColor::white));
            lightActive.setColor(QPalette::AlternateBase, QColor(0xee, 0xee, 0xee));
            lightActive.setColor(QPalette::Highlight, QColor(0x0d, 0xbd, 0x8b));
            lightActive.setColor(QPalette::HighlightedText, QColor(0xf4, 0xf4, 0xf5));
            lightActive.setColor(QPalette::ToolTipBase, lightActive.base().color());
            lightActive.setColor(QPalette::ToolTipText, lightActive.text().color());
            lightActive.setColor(QPalette::Link, QColor(0x05, 0x8a, 0x67));
            lightActive.setColor(QPalette::ButtonText, QColor(0x55, 0x54, 0x59));
            return lightActive;
        }();
        return lightActive;
    } else if (theme == u"dark") {
        static QPalette darkActive = [] {
            QPalette darkActive(
              /*windowText*/ QColor(0xca, 0xcc, 0xd1),
              /*button*/ QColor(Qt::GlobalColor::white),
              /*light*/ QColor(0xca, 0xcc, 0xd1),
              /*dark*/ QColor(60, 70, 77),
              /*mid*/ QColor(0x20, 0x22, 0x28),
              /*text*/ QColor(0xca, 0xcc, 0xd1),
              /*bright_text*/ QColor(0xf4, 0xf5, 0xf8),
              /*base*/ QColor(0x20, 0x22, 0x28),
              /*window*/ QColor(0x2d, 0x31, 0x39));
            darkActive.setColor(QPalette::AlternateBase, QColor(0x34, 0x39, 0x42));
            // The 9-arg constructor above leaves Button hardcoded to white; stock
            // QtQuick Controls (e.g. ComboBox) actually use it, unlike our own
            // widgets, so it needs an explicit dark-appropriate color here.
            darkActive.setColor(QPalette::Button, QColor(60, 70, 77));
            darkActive.setColor(QPalette::Highlight, QColor(0x0d, 0xbd, 0x8b));
            darkActive.setColor(QPalette::HighlightedText, QColor(0xf4, 0xf5, 0xf8));
            darkActive.setColor(QPalette::ToolTipBase, darkActive.base().color());
            darkActive.setColor(QPalette::ToolTipText, darkActive.text().color());
            darkActive.setColor(QPalette::Link, QColor(0x1a, 0xd1, 0x9f));
            darkActive.setColor(QPalette::ButtonText, QColor(0x82, 0x82, 0x84));
            return darkActive;
        }();
        return darkActive;
    } else if (theme == u"system") {
        return original;
    } else if (auto custom = ThemeLoader::load(theme.toString())) {
        return custom->palette;
    } else {
        return original;
    }
}

Theme::Theme(QStringView theme)
{
    auto p     = paletteFromTheme(theme);
    separator_ = p.mid().color();
    if (theme == u"light") {
        sidebarBackground_ = QColor(0x23, 0x36, 0x49);
        alternateButton_   = QColor(0xcc, 0xcc, 0xcc);
        red_               = QColor(0xa8, 0x23, 0x53);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(0xfc, 0xbe, 0x05);
        error_             = QColor(0xdd, 0x3d, 0x3d);
    } else if (theme == u"dark") {
        // Was the same color as window (below) - the communities rail, room list, and timeline
        // all rendered as one indistinguishable block. Reuses AlternateBase's shade, the same
        // "one step up from window" color already used elsewhere in this theme for elevated
        // surfaces, rather than inventing a new one.
        sidebarBackground_ = QColor(0x34, 0x39, 0x42);
        alternateButton_   = QColor(0x41, 0x4A, 0x59);
        red_               = QColor(0xa8, 0x23, 0x53);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(0xfc, 0xc5, 0x3a);
        error_             = QColor(0xdd, 0x3d, 0x3d);
    } else if (theme == u"system") {
        // Falling back to the inherited system palette otherwise leaves sidebarBackground
        // identical to the window color it's meant to be distinguishable from - the sidebar,
        // room list, and timeline all end up the exact same shade, with no visual cue that
        // they're separate areas. Nudge it a little instead: darker for a light system theme,
        // lighter for a dark one, so there's always some contrast regardless of what the
        // system palette actually looks like.
        auto windowColor   = p.window().color();
        sidebarBackground_ = windowColor.lightness() > 128 ? windowColor.darker(112) : windowColor.lighter(122);
        alternateButton_   = p.dark().color();
        red_               = QColor(QColorConstants::Svg::red);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(QColorConstants::Svg::orange); // SVG orange
        error_             = QColor(0xdd, 0x3d, 0x3d);
    } else if (auto custom = ThemeLoader::load(theme.toString())) {
        sidebarBackground_ = custom->sidebarBackground;
        alternateButton_   = custom->alternateButton;
        red_               = custom->red;
        green_             = custom->green;
        orange_            = custom->orange;
        error_             = custom->error;
    } else {
        // A theme name that isn't a built-in and isn't a loadable custom theme either (a typo,
        // or one whose file was deleted) - fall back to the same system-derived contrast as the
        // "system" branch above rather than leaving these uninitialized.
        auto windowColor   = p.window().color();
        sidebarBackground_ = windowColor.lightness() > 128 ? windowColor.darker(112) : windowColor.lighter(122);
        alternateButton_   = p.dark().color();
        red_               = QColor(QColorConstants::Svg::red);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(QColorConstants::Svg::orange); // SVG orange
        error_             = QColor(0xdd, 0x3d, 0x3d);
    }
}

#include "moc_Theme.cpp"
