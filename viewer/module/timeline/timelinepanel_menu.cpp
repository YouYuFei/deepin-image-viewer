#include "timelinepanel.h"
#include "timelineimageview.h"
#include "controller/dbmanager.h"
#include "controller/exporter.h"
#include "controller/popupmenumanager.h"
#include "controller/wallpapersetter.h"
#include "frame/deletedialog.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include <QShortcut>
#include <QtConcurrent>

namespace {

const QString FAVORITES_ALBUM_NAME = "My favorites";
const QString SHORTCUT_SPLIT_FLAG = "@-_-@";

const QString SETTINGS_GROUP = "TIMEPANEL";
const QString SETTINGS_ICON_SCALE_KEY = "IconScale";

enum MenuItemId {
    IdView,
    IdFullScreen,
    IdStartSlideShow,
    IdAddToAlbum,
    IdExport,
    IdCopy,
    IdMoveToTrash,
    IdRemoveFromTimeline,
    IdEdit,
    IdAddToFavorites,
    IdRemoveFromFavorites,
    IdRotateClockwise,
    IdRotateCounterclockwise,
    IdLabel,
    IdSetAsWallpaper,
    IdDisplayInFileManager,
    IdImageInfo,
    IdSubMenu,
    IdSeparator
};

void createMI(QJsonArray *items,
              const MenuItemId id,
              const QString &text,
              const QString &shortcut = "",
              const bool isSeparator = false,
              const QJsonObject &subMenu = QJsonObject())
{
    auto obj = PopupMenuManager::createItemObj(int(id), text,
                                               isSeparator, shortcut, subMenu);
    items->append(obj);
}

}  // namespace

void TimelinePanel::initPopupMenu()
{
    m_popupMenu = new PopupMenuManager(this);
    updateMenuContents();
    connect(m_popupMenu, &PopupMenuManager::menuItemClicked,
            this, &TimelinePanel::onMenuItemClicked);
}

void TimelinePanel::initShortcut()
{
    // Image info
    QShortcut *sc = new QShortcut(QKeySequence("Alt+Return"), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [=] {
        const QStringList ePaths = m_view->selectedPaths();
        if (! ePaths.isEmpty()) {
            dApp->signalM->showImageInfo(ePaths.first());
        }
    });

    // Select all
    sc = new QShortcut(QKeySequence("Ctrl+A"), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [=] {
        m_view->selectAll();
    });
}

void TimelinePanel::updateMenuContents()
{
    // For update shortcut
    m_popupMenu->setMenuContent(createMenuContent());
}

void TimelinePanel::onMenuItemClicked(int menuId, const QString &text)
{
    const QStringList dPaths = m_view->selectedPaths(false);
    const QStringList ePaths = m_view->selectedPaths();
    if (ePaths.isEmpty()) {
        return;
    }

    const QStringList viewPaths = (ePaths.length() == 1) ?
                dApp->dbM->getAllPaths() : ePaths;
    const QString epath = ePaths.first();
    const QString dpath = dPaths.first();

    SignalManager::ViewInfo vinfo;
    vinfo.inDatabase = true;
    vinfo.lastPanel = this;
    vinfo.path = epath;
    vinfo.paths = viewPaths;

    switch (MenuItemId(menuId)) {
    case IdView:
        dApp->signalM->viewImage(vinfo);
        break;
    case IdFullScreen:
        vinfo.fullScreen = true;
        dApp->signalM->viewImage(vinfo);
        break;
    case IdStartSlideShow:
        dApp->signalM->startSlideShow(vinfo);
        break;
    case IdAddToAlbum: {
        const QString album = text.split(SHORTCUT_SPLIT_FLAG).first();
        dApp->dbM->insertIntoAlbum(album, ePaths);
        break;
    }
    case IdCopy:
        utils::base::copyImageToClipboard(dPaths);
        break;
    case IdMoveToTrash: {
        popupDelDialog(dPaths, ePaths);
        break;
    }
    case IdAddToFavorites:
        dApp->dbM->insertIntoAlbum(FAVORITES_ALBUM_NAME, ePaths);
        updateMenuContents();
        break;
    case IdRemoveFromFavorites:
        dApp->dbM->removeFromAlbum(FAVORITES_ALBUM_NAME, ePaths);
        updateMenuContents();
        break;
    case IdRotateClockwise:
        if (m_rotateList.isEmpty()) {
            m_rotateList = dPaths;
            for (QString path : dPaths) {
                QtConcurrent::run(this, &TimelinePanel::rotateImage, path, 90);
            }
        }
        break;
    case IdRotateCounterclockwise:
        if (m_rotateList.isEmpty()) {
            m_rotateList = ePaths;
            for (QString path : dPaths) {
                QtConcurrent::run(this, &TimelinePanel::rotateImage, path, -90);
            }
        }
        break;
    case IdSetAsWallpaper:
        dApp->wpSetter->setWallpaper(dpath);
        break;
    case IdDisplayInFileManager:
        utils::base::showInFileManager(dpath);
        break;
    case IdImageInfo:
        dApp->signalM->showImageInfo(dpath);
        break;
    default:
        break;
    }
}

QString TimelinePanel::createMenuContent()
{
    auto dPaths = m_view->selectedPaths(false);
    auto ePaths = m_view->selectedPaths();
    auto supportPath = std::find_if_not(dPaths.cbegin(), dPaths.cend(),
                                        utils::image::imageSupportSave);
    bool canSave = supportPath == dPaths.cend();

    QJsonArray items;

    ////////////////////////////////////////////////////////////////////////////
    if (dPaths.length() == 1) {
        createMI(&items, IdView, tr("View"));
        createMI(&items, IdFullScreen, tr("Fullscreen"), "F11");
    }
    createMI(&items, IdStartSlideShow, tr("Start slide show"), "F5");
    const QJsonObject objF = createAlbumMenuObj();
    if (! objF.isEmpty()) {
        createMI(&items, IdAddToAlbum, tr("Add to album"), "", false, objF);
    }
    createMI(&items, IdSeparator, "", "", true);

    ////////////////////////////////////////////////////////////////////////////
    createMI(&items, IdCopy, tr("Copy"), "Ctrl+C");
    createMI(&items, IdMoveToTrash, tr("Throw to Trash"), "Delete");
    createMI(&items, IdSeparator, "", "", true);

    ////////////////////////////////////////////////////////////////////////////
    if (dPaths.length() == 1) {
        if (! dApp->dbM->isImgExistInAlbum(FAVORITES_ALBUM_NAME, ePaths.first())) {
            createMI(&items, IdAddToFavorites, tr("Add to My favorites"),
                     "Ctrl+K");
        }
        else {
            createMI(&items, IdRemoveFromFavorites, tr("Unfavorite"),
                     "Ctrl+Shift+K");
        }
    } else {
        bool v = false;
        for (QString img : ePaths) {
            if (! dApp->dbM->isImgExistInAlbum(FAVORITES_ALBUM_NAME, img)) {
                v = true;
                break;
            }
        }
        if (v) {
            createMI(&items, IdAddToFavorites, tr("Add to My favorites"), "Ctrl+K");
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    if (canSave) {
        createMI(&items, IdSeparator, "", "", true);
        createMI(&items, IdRotateClockwise, tr("Rotate clockwise"), "Ctrl+R");
        createMI(&items, IdRotateCounterclockwise, tr("Rotate counterclockwise"), "Ctrl+Shift+R");
    }
    createMI(&items, IdSeparator, "", "", true);

    ////////////////////////////////////////////////////////////////////////////
    if (dPaths.length() == 1) {
        if (canSave) {
            createMI(&items, IdSetAsWallpaper, tr("Set as wallpaper"), "Ctrl+F8");
        }
        createMI(&items, IdDisplayInFileManager, tr("Display in file manager"), "Ctrl+D");
        createMI(&items, IdImageInfo, tr("Image info"), "Alt+Enter");
    }

    QJsonObject contentObj;
    contentObj["x"] = 0;
    contentObj["y"] = 0;
    contentObj["items"] = QJsonValue(items);

    return QString(QJsonDocument(contentObj).toJson());
}

QJsonObject TimelinePanel::createAlbumMenuObj()
{
    const QStringList albums = dApp->dbM->getAllAlbumNames();
    const QStringList ePaths = m_view->selectedPaths();

    QJsonArray items;
    if (! ePaths.isEmpty()) {
        for (QString album : albums) {
            if (album == FAVORITES_ALBUM_NAME) {
                continue;
            }
            const QStringList aps = dApp->dbM->getPathsByAlbum(album);
            for (QString path : ePaths) {
                if (aps.indexOf(path) == -1) {
                    createMI(&items, IdAddToAlbum, album);
                    break;
                }
            }
        }
    }

    QJsonObject contentObj;
    if (! items.isEmpty()) {
        contentObj[""] = QJsonValue(items);
    }

    return contentObj;
}

void TimelinePanel::rotateImage(const QString &path, int degree)
{
    utils::image::rotate(path, degree);
    m_rotateList.removeAll(path);
    if (m_rotateList.isEmpty()) {
        qDebug() << "Rotate finish!";
        m_view->updateThumbnails();
    }
}

void TimelinePanel::popupDelDialog(const QStringList &dpaths, const QStringList &epaths)
{
    DeleteDialog* dialog = new DeleteDialog(dpaths, false, this);
    dialog->show();
    dialog->moveToCenter();
    connect(dialog, &DeleteDialog::buttonClicked, [=](int index){
        if (index == 1) {
            dApp->dbM->removeImgInfos(epaths);
            utils::base::trashFiles(dpaths);
        }
    });
    connect(dialog, &DeleteDialog::closed, dialog, &DeleteDialog::deleteLater);

}
