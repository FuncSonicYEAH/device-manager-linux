#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQmlError>
#include <QFontDatabase>
#include <QFontInfo>
#include <QTimer>
#include <cstdio>

#include "ColorUtils.h"
#include "Theme.h"
#include "DeviceManager.h"
#include "DeviceActions.h"
#include "SmartReader.h"
#include "GraphicsProbe.h"
#include "AboutInfo.h"
#include "Translator.h"

// Bundle the Material Symbols Rounded variable font so the icon widget works
// without a system-wide fontconfig installation. Same approach as m3-gallery:
// register it straight from the Qt resource.
static int registerBundledFont()
{
    return QFontDatabase::addApplicationFont(
        QStringLiteral(":/qml/fonts/MaterialSymbolsRounded.ttf"));
}

int main(int argc, char *argv[])
{
    // Default to the Wayland platform when available, falling back to X11
    // (or offscreen/minimal for headless debugging). An explicit
    // QT_QPA_PLATFORM in the environment always wins.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "wayland;xcb");

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("device-manager"));
    QCoreApplication::setApplicationName(QStringLiteral("device-manager"));
    Translator translator;
    Translator::setInstance(&translator);

    int fontId = -1;
    if (!qEnvironmentVariableIsSet("DM_NO_FONT"))
        fontId = registerBundledFont();

    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--fontinfo")) {
        printf("addApplicationFont id: %d\n", fontId);
        const QStringList fams = QFontDatabase::applicationFontFamilies(fontId);
        for (const QString &fam : fams)
            printf("applicationFontFamilies: %s\n", qPrintable(fam));
        printf("families() contains 'Material Symbols Rounded': %s\n",
               QFontDatabase::families().contains(QStringLiteral("Material Symbols Rounded")) ? "yes" : "no");
        QFont fnt;
        fnt.setFamily(QStringLiteral("Material Symbols Rounded"));
        QFontInfo fi(fnt);
        printf("QFont resolved family: %s\n", qPrintable(fi.family()));
        return 0;
    }

    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--gpuinfo")) {
        GraphicsProbe probe;
        const QVariantList gpus = probe.gpus();
        printf("GPUs: %d\n", int(gpus.size()));
        for (const QVariant &gv : gpus) {
            const QVariantMap g = gv.toMap();
            printf("  %-32s | %-12s | %-12s | GL:%-3s | %-22s | VK:%-3s | %s\n",
                   qPrintable(g.value(QStringLiteral("name")).toString()),
                   qPrintable(g.value(QStringLiteral("vendor")).toString()),
                   qPrintable(g.value(QStringLiteral("driver")).toString()),
                   g.value(QStringLiteral("glSupported")).toBool() ? "yes" : "no",
                   qPrintable(g.value(QStringLiteral("glNote")).toString()),
                   g.value(QStringLiteral("vkSupported")).toBool() ? "yes" : "no",
                   qPrintable(g.value(QStringLiteral("vkNote")).toString()));
        }
        printf("OpenGL: supported=%s providers=[%s] renderer='%s' version='%s'\n",
               probe.openGLSupported() ? "yes" : "no",
               qPrintable(probe.openGLProviders().join(QStringLiteral(", "))),
               qPrintable(probe.openGLRenderer()),
               qPrintable(probe.openGLVersion()));
        printf("Vulkan: supported=%s drivers=[%s] apiVersion='%s'\n",
               probe.vulkanSupported() ? "yes" : "no",
               qPrintable(probe.vulkanDrivers().join(QStringLiteral(", "))),
               qPrintable(probe.vulkanApiVersion()));
        return 0;
    }

    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--dump")) {
        DeviceManager dm;
        const QVariantList groups = dm.typeGroups();
        for (const QVariant &gv : groups) {
            const QVariantMap g = gv.toMap();
            const QVariantList devs = g.value(QStringLiteral("devices")).toList();
            printf("\n== %s (%d) ==\n", qPrintable(g.value(QStringLiteral("name")).toString()), int(devs.size()));
            for (const QVariant &dv : devs) {
                const QVariantMap d = dv.toMap();
                printf("  %-42s | %-14s | %-12s | %-9s | %s\n",
                       qPrintable(d.value(QStringLiteral("name")).toString()),
                       qPrintable(d.value(QStringLiteral("vendor")).toString()),
                       qPrintable(d.value(QStringLiteral("driver")).toString()),
                       qPrintable(d.value(QStringLiteral("status")).toString()),
                       qPrintable(d.value(QStringLiteral("id")).toString()));
            }
        }
        printf("\nTotal: %d devices, %d problems\n", dm.deviceCount(), dm.problemCount());
        return 0;
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Optional: force a UI language for testing (e.g. --lang en)
    for (int i = 1; i < argc - 1; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == QLatin1String("--lang"))
            Translator::instance()->setLanguage(QString::fromLocal8Bit(argv[i + 1]), false);
    }

    ColorUtils colorUtils;
    Theme theme;
    DeviceManager deviceManager;
    SmartReader smartReader;
    DeviceActions deviceActions;
    GraphicsProbe graphicsProbe;
    AboutInfo aboutInfo;
    // re-enumerate so the device groups are rebuilt in the new language
    bool noRefresh = false;
    for (int i = 1; i < argc; ++i)
        if (QString::fromLocal8Bit(argv[i]) == QLatin1String("--no-refresh"))
            noRefresh = true;
    if (!noRefresh)
        QObject::connect(Translator::instance(), &Translator::languageChanged,
                         &deviceManager, &DeviceManager::refresh);

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QObject::connect(&engine, &QQmlEngine::warnings,
                     [](const QList<QQmlError> &warnings) {
                         for (const QQmlError &w : warnings)
                             fprintf(stderr, "QML: %s:%d: %s\n",
                                     w.url().toString().toUtf8().constData(),
                                     w.line(), qPrintable(w.description()));
                     });
    engine.rootContext()->setContextProperty(QStringLiteral("ColorUtils"), &colorUtils);
    engine.rootContext()->setContextProperty(QStringLiteral("Appearance"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("DeviceManager"), &deviceManager);
    engine.rootContext()->setContextProperty(QStringLiteral("Smart"), &smartReader);
    engine.rootContext()->setContextProperty(QStringLiteral("DeviceActions"), &deviceActions);
    engine.rootContext()->setContextProperty(QStringLiteral("Graphics"), &graphicsProbe);
    engine.rootContext()->setContextProperty(QStringLiteral("AboutInfo"), &aboutInfo);
    engine.rootContext()->setContextProperty(QStringLiteral("Tr"), Translator::instance());

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    if (argc > 2 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--screenshot")) {
        // debug helper: --light forces the light scheme for the capture
        for (int i = 1; i < argc; ++i)
            if (QString::fromLocal8Bit(argv[i]) == QLatin1String("--light"))
                theme.setDarkMode(false);
        // debug helper: --switch <code> switches the language at runtime after
        // the first capture and takes a second one (retranslate check)
        QString switchLang;
        bool noDialog = false;
        for (int i = 1; i < argc - 1; ++i) {
            if (QString::fromLocal8Bit(argv[i]) == QLatin1String("--switch"))
                switchLang = QString::fromLocal8Bit(argv[i + 1]);
            if (QString::fromLocal8Bit(argv[i]) == QLatin1String("--no-dialog"))
                noDialog = true;
        }
        QTimer::singleShot(900, [&]() {
            if (auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
                // select the first device so the details pane is populated
                const QVariantList groups = deviceManager.typeGroups();
                if (!groups.isEmpty()) {
                    const QVariantMap g = groups.first().toMap();
                    const QVariantList devs = g.value(QStringLiteral("devices")).toList();
                    if (!noDialog && !devs.isEmpty())
                        QMetaObject::invokeMethod(engine.rootObjects().first(),
                                                  "selectDevice",
                                                  Q_ARG(QVariant, devs.first()));
                    // open the properties dialog to verify its styling
                    if (!noDialog)
                        QMetaObject::invokeMethod(engine.rootObjects().first(), "openProperties");
                }
                const QImage img = win->grabWindow();
                img.save(QString::fromLocal8Bit(argv[2]));
                if (QString::fromLocal8Bit(argv[1]) == QLatin1String("--switch-theme")) {
                    theme.setDarkMode(!theme.isDark());
                    theme.setDummy(QStringLiteral("y"));
                    QTimer::singleShot(3000, []() { qApp->quit(); });
                    return;
                }
                if (QString::fromLocal8Bit(argv[1]) == QLatin1String("--switch-qml")) {
                    QMetaObject::invokeMethod(engine.rootObjects().first(), "setProperty",
                                              Q_ARG(QString, QStringLiteral("viewMode")),
                                              Q_ARG(QVariant, QStringLiteral("connection")));
                    QTimer::singleShot(3000, []() { qApp->quit(); });
                    return;
                }
                if (!switchLang.isEmpty()) {
                    Translator::instance()->setLanguage(switchLang, false);
                    if (qEnvironmentVariableIsSet("DM_NO_GRAB2"))
                        QTimer::singleShot(3000, []() { qApp->quit(); });
                    else
                        QTimer::singleShot(3000, [win, path = QString::fromLocal8Bit(argv[2])]() {
                            win->grabWindow().save(path + QStringLiteral(".switched.png"));
                            qApp->quit();
                        });
                    return;
                }
            }
            app.quit();
        });
    }

    return app.exec();
}
