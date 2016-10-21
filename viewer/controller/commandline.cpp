#include "commandline.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "controller/divdbuscontroller.h"
#include "frame/mainwindow.h"
#include "utils/imageutils.h"
#include "utils/baseutils.h"
#include <QCommandLineOption>
#include <QDBusConnection>
#include <QDebug>
#include <QFileInfo>

namespace {

const QString DBUS_PATH = "/com/deepin/deepinimageviewer";
const QString DBUS_NAME = "com.deepin.deepinimageviewer";

}

struct CMOption {
   QString shortOption;
   QString longOption;
   QString description;
   QString valueName;
};

static CMOption options[] = {
    {"o", "open", "Open the specified <image-file>.", "image-file"},
    {"a", "album", "Enter the album <album-name>.", "album-name"},
//    {"s", "search", "Go to search view and search image by <word>.", "word"},
//    {"e", "edit", "Go to edit view and begin editing <image-file>.", "image-file"},
    {"w", "wallpaper", "Set <image-file> as wallpaper.", "image-file"},
    {"", "", "", ""}
};

CommandLine *CommandLine::m_commandLine = nullptr;
CommandLine *CommandLine::instance()
{
    if (! m_commandLine) {
        m_commandLine = new CommandLine();
    }

    return m_commandLine;
}

CommandLine::CommandLine()
{
    m_cmdParser.addHelpOption();
//    m_cmdParser.addVersionOption();
    m_cmdParser.addPositionalArgument("value", QCoreApplication::translate(
        "main", "Value that use for options."), "[value]");

    for (const CMOption* i = options; ! i->shortOption.isEmpty(); ++i) {
        addOption(i);
    }
}

CommandLine::~CommandLine() {

}

void CommandLine::addOption(const CMOption *option)
{
    QStringList ol;
    ol << option->shortOption;
    ol << option->longOption;
    QCommandLineOption cm(ol, option->description, option->valueName);

    m_cmdParser.addOption(cm);
}

/*!
 * \brief CommandLine::showHelp
 * QCommandLineParser::showHelp(int exitCode = 0) Will displays the help
 * infomation, and exits application automatically. However,
 * DApplication::loadDXcbPlugin() need exit by calling quick_exit(a.exec()).
 * So we should show the help message only by calling this function.
 */
void CommandLine::showHelp()
{
    fputs(qPrintable(m_cmdParser.helpText()), stdout);
}

void CommandLine::viewImage(const QString &path, const QStringList &paths)
{
    MainWindow *w = new MainWindow(false);
    w->show();
    // Load image after all UI elements has been init
    // BottomToolbar pos not correct on init
    emit dApp->signalM->hideBottomToolbar(true);
    emit dApp->signalM->enableMainMenu(false);
    QStringList ePaths;
    for (QString dp : paths) {
        ePaths << dp.toUtf8().toPercentEncoding("/");
    }
    TIMER_SINGLESHOT(50, {
    SignalManager::ViewInfo vinfo;
    vinfo.album = "";
    vinfo.inDatabase = false;
    vinfo.lastPanel = nullptr;
    vinfo.path = path.toUtf8().toPercentEncoding("/");
    vinfo.paths = ePaths;
    emit dApp->signalM->viewImage(vinfo);
                     }, path, ePaths)
}

bool CommandLine::processOption()
{
    if (! m_cmdParser.parse(dApp->arguments())) {
        showHelp();
        return false;
    }

    DeepinImageViewerDBus *dd = new DeepinImageViewerDBus(dApp->signalM);
    Q_UNUSED(dd);

    QStringList names = m_cmdParser.optionNames();
    QStringList pas = m_cmdParser.positionalArguments();
    if (names.isEmpty() && pas.isEmpty()) {
        if (QDBusConnection::sessionBus().registerService(DBUS_NAME) &&
                QDBusConnection::sessionBus().registerObject(DBUS_PATH, dApp->signalM)) {
            MainWindow *w = new MainWindow(true);

            w->show();
            emit dApp->signalM->backToMainPanel();

            return true;
        }
        else {
            qDebug() << "Deepin Image Viewer is running...";
            return false;
        }
    }
    else {
        using namespace utils::image;
        DIVDBusController *dc = new DIVDBusController(dApp->signalM);
        Q_UNUSED(dc)

        QString name;
        QString value;
        QStringList values;
        if (! names.isEmpty()) {
            name = names.first();
            value = m_cmdParser.value(name);
            values = m_cmdParser.values(name);
        }
        else if (! pas.isEmpty()){
            name = "o"; // Default operation is open image file
            value = pas.first();
            values = pas;
        }

        bool support = imageSupportRead(value);

        if (name == "o" || name == "open") {
            if (values.length() > 1) {
                QStringList aps;
                for (QString path : values) {
                    const QString ap = QFileInfo(path).absoluteFilePath();
                    if (QFileInfo(path).exists() && imageSupportRead(ap)) {
                        aps << ap;
                    }
                }
                if (! aps.isEmpty()) {
                    viewImage(aps.first(), aps);
                    return true;
                }
                else {
                    return false;
                }
            }
            else if (imageSupportRead(QFileInfo(value).absoluteFilePath())) {
                viewImage(QFileInfo(value).absoluteFilePath(), QStringList());
                return true;
            }
            else {
                return false;
            }
        }
        else if (name == "a" || name == "album") {
            dc->enterAlbum(value);
        }
        else if (name == "s" || name == "search") {
            dc->searchImage(value);
        }
        else if ((name == "e" || name == "edit") && support) {
            dc->editImage(QFileInfo(value).absoluteFilePath());
        }
        else if ((name == "w" || name == "wallpaper") && support) {
            qDebug() << "Set " << value << " as wallpaper.";
            dApp->wpSetter->setWallpaper(QFileInfo(value).absoluteFilePath());
        }
        else {
            showHelp();
        }

        return false;
    }
}

DeepinImageViewerDBus::DeepinImageViewerDBus(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{

}

DeepinImageViewerDBus::~DeepinImageViewerDBus()
{

}

void DeepinImageViewerDBus::backToMainWindow() const
{
    emit dApp->signalM->backToMainPanel();
}

void DeepinImageViewerDBus::enterAlbum(const QString &album)
{
    qDebug() << "Enter the album: " << album;
    // TODO
}

void DeepinImageViewerDBus::searchImage(const QString &keyWord)
{
    qDebug() << "Go to search view and search image by: " << keyWord;
    // TODO
}

void DeepinImageViewerDBus::editImage(const QString &path)
{
    qDebug() << "Go to edit view and begin editing: " << path;
    emit dApp->signalM->editImage(path);
}
