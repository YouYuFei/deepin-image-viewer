#if 0
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QApplication>
#include <DLog>
#include <QTranslator>
#include <DApplicationSettings>
#include "application.h"
#include "controller/commandline.h"
#include "service/defaultimageviewer.h"
#include <QTimer>
using namespace Dtk::Core;
int main(int argc, char *argv[])
{

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    Application::loadDXcbPlugin();
    Application a(argc,argv);
    a.setAttribute(Qt::AA_ForceRasterWidgets);
    testing::InitGoogleTest();
    if (!service::isDefaultImageViewer()) {
        qDebug() << "Set defaultImage viewer succeed:" << service::setDefaultImageViewer(true);
    } else {
        qDebug() << "Deepin Image Viewer is defaultImage!";
    }
    RUN_ALL_TESTS();

    return a.exec();
//  return  RUN_ALL_TESTS();
//return a.exec();

}
#else
/*
* Copyright (C) 2019 ~ 2020 UnionTech Software Technology Co.,Ltd
*
* Author:     liuminghang<liuminghang@uniontech.com>
* Maintainer: liuminghang<liuminghang@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QtTest>
#include <QCoreApplication>
#include "application.h"

#include <QApplication>
#include <DLog>
#include <QTranslator>
#include <DApplicationSettings>
#include "application.h"
#include "controller/commandline.h"
#include "service/defaultimageviewer.h"
#include <QTimer>


#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#define QMYTEST_MAIN(TestObject) \
    QT_BEGIN_NAMESPACE \
    QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS \
    QT_END_NAMESPACE \
    int main(int argc, char *argv[]) \
    { \
        Application app(argc, argv); \
        app.setAttribute(Qt::AA_Use96Dpi, true); \
        QTEST_DISABLE_KEYPAD_NAVIGATION \
        QTEST_ADD_GPU_BLACKLIST_SUPPORT \
        TestObject tc; \
        QTEST_SET_MAIN_SOURCE_PATH \
        return QTest::qExec(&tc, argc, argv); \
    }

class QTestMain : public QObject
{
    Q_OBJECT

public:
    QTestMain();
    ~QTestMain();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testGTest();
};

QTestMain::QTestMain()
{

}

QTestMain::~QTestMain()
{

}

void QTestMain::initTestCase()
{
}

void QTestMain::cleanupTestCase()
{

}

void QTestMain::testGTest()
{
    testing::InitGoogleTest();
    int ret = RUN_ALL_TESTS();
    Q_UNUSED(ret)
}

QMYTEST_MAIN(QTestMain)

#include "main.moc"
#endif