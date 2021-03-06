/*
 * Hedgewars, a free turn based strategy game
 * Copyright (c) 2006-2011 Igor Ulyanov <iulyanov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <QPushButton>
#include <QBuffer>
#include <QUuid>
#include <QBitmap>
#include <QPainter>
#include <QLinearGradient>
#include <QColor>
#include <QTextStream>
#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMessageBox>

#include "hwconsts.h"
#include "mapContainer.h"
#include "igbox.h"

HWMapContainer::HWMapContainer(QWidget * parent) :
    QWidget(parent),
    mainLayout(this),
    pMap(0),
    mapgen(MAPGEN_REGULAR)
{
    hhSmall.load(":/res/hh_small.png");
    hhLimit = 18;
    templateFilter = 0;

    mainLayout.setContentsMargins(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
        1,
        QApplication::style()->pixelMetric(QStyle::PM_LayoutRightMargin),
        QApplication::style()->pixelMetric(QStyle::PM_LayoutBottomMargin));

    QWidget* mapWidget = new QWidget(this);
    mainLayout.addWidget(mapWidget, 0, 0, Qt::AlignHCenter);

    QGridLayout* mapLayout = new QGridLayout(mapWidget);
    mapLayout->setMargin(0);

    imageButt = new QPushButton(mapWidget);
    imageButt->setObjectName("imageButt");
    imageButt->setFixedSize(256 + 6, 128 + 6);
    imageButt->setFlat(true);
    imageButt->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);//QSizePolicy::Minimum, QSizePolicy::Minimum);
    mapLayout->addWidget(imageButt, 0, 0, 1, 2);
    //connect(imageButt, SIGNAL(clicked()), this, SLOT(setRandomSeed()));
    //connect(imageButt, SIGNAL(clicked()), this, SLOT(setRandomTheme()));
    connect(imageButt, SIGNAL(clicked()), this, SLOT(setRandomMap()));

    chooseMap = new QComboBox(mapWidget);
    chooseMap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    chooseMap->addItem(
// FIXME - need real icons. Disabling until then
//QIcon(":/res/mapRandom.png"), 
QComboBox::tr("generated map..."));
    chooseMap->addItem(
// FIXME - need real icons. Disabling until then
//QIcon(":/res/mapMaze.png"), 
QComboBox::tr("generated maze..."));

    chooseMap->addItem(QComboBox::tr("hand drawn map..."));
    chooseMap->insertSeparator(chooseMap->count()); // separator between generators and missions

    chooseMap->insertSeparator(chooseMap->count()); // separator between generators and missions

    int missionindex = chooseMap->count();
    numMissions = 0;
    for (int i = 0; i < mapList->size(); ++i) {
        QString map = (*mapList)[i];
        QFile mapCfgFile(
                QString("%1/Maps/%2/map.cfg")
                .arg(datadir->absolutePath())
                .arg(map));
        QFile mapLuaFile(
                QString("%1/Maps/%2/map.lua")
                .arg(datadir->absolutePath())
                .arg(map));

        if (mapCfgFile.open(QFile::ReadOnly)) {
            QString theme;
            quint32 limit = 0;
            QString scheme;
            QString weapons;
            QList<QVariant> mapInfo;
            QTextStream input(&mapCfgFile);
            input >> theme;
            input >> limit;
            input >> scheme;
            input >> weapons;
            mapInfo.push_back(map);
            mapInfo.push_back(theme);
            if (limit)
                mapInfo.push_back(limit);
            else
                mapInfo.push_back(18);
            mapInfo.push_back(mapLuaFile.exists());
            if (scheme.isEmpty())
                scheme = "locked";
            scheme.replace("_", " ");
            if (weapons.isEmpty())
                weapons = "locked";
            weapons.replace("_", " ");
            mapInfo.push_back(scheme);
            mapInfo.push_back(weapons);
            if(mapLuaFile.exists())
            {
                chooseMap->insertItem(missionindex++, 
// FIXME - need real icons. Disabling until then
//QIcon(":/res/mapMission.png"), 
QComboBox::tr("Mission") + ": " + map, mapInfo);
                numMissions++;
            }
            else
                chooseMap->addItem(
// FIXME - need real icons. Disabling until then
//QIcon(":/res/mapCustom.png"), 
map, mapInfo);
            mapCfgFile.close();
        }
    }
    chooseMap->insertSeparator(missionindex); // separator between missions and maps

    connect(chooseMap, SIGNAL(activated(int)), this, SLOT(mapChanged(int)));
    mapLayout->addWidget(chooseMap, 1, 1);

    QLabel * lblMap = new QLabel(tr("Map"), mapWidget);
    mapLayout->addWidget(lblMap, 1, 0);

    lblFilter = new QLabel(tr("Filter"), mapWidget);
    mapLayout->addWidget(lblFilter, 2, 0);

    cbTemplateFilter = new QComboBox(mapWidget);
    cbTemplateFilter->addItem(tr("All"), 0);
    cbTemplateFilter->addItem(tr("Small"), 1);
    cbTemplateFilter->addItem(tr("Medium"), 2);
    cbTemplateFilter->addItem(tr("Large"), 3);
    cbTemplateFilter->addItem(tr("Cavern"), 4);
    cbTemplateFilter->addItem(tr("Wacky"), 5);
    mapLayout->addWidget(cbTemplateFilter, 2, 1);

    connect(cbTemplateFilter, SIGNAL(activated(int)), this, SLOT(setTemplateFilter(int)));

    maze_size_label = new QLabel(tr("Type"), mapWidget);
    mainLayout.addWidget(maze_size_label, 2, 0);
    maze_size_label->hide();
    cbMazeSize = new QComboBox(mapWidget);
    cbMazeSize->addItem(tr("Small tunnels"), 0);
    cbMazeSize->addItem(tr("Medium tunnels"), 1);
    cbMazeSize->addItem(tr("Large tunnels"), 2);
    cbMazeSize->addItem(tr("Small floating islands"), 3);
    cbMazeSize->addItem(tr("Medium floating islands"), 4);
    cbMazeSize->addItem(tr("Large floating islands"), 5);
    cbMazeSize->setCurrentIndex(1);

    mapLayout->addWidget(cbMazeSize, 2, 1);
    cbMazeSize->hide();
    connect(cbMazeSize, SIGNAL(activated(int)), this, SLOT(setMazeSize(int)));

    gbThemes = new IconedGroupBox(mapWidget);
    gbThemes->setTitleTextPadding(80);
    gbThemes->setContentTopPadding(15);
    gbThemes->setTitle(tr("Themes"));

    //gbThemes->setStyleSheet("padding: 0px"); // doesn't work - stylesheet is set with icon
    mapLayout->addWidget(gbThemes, 0, 2, 3, 1);

    QVBoxLayout * gbTLayout = new QVBoxLayout(gbThemes);
    gbTLayout->setContentsMargins(0, 0, 0 ,0);
    gbTLayout->setSpacing(0);
    lwThemes = new QListWidget(mapWidget);
    lwThemes->setMinimumHeight(30);
    lwThemes->setFixedWidth(140);
    for (int i = 0; i < Themes->size(); ++i) {
        QListWidgetItem * lwi = new QListWidgetItem();
        lwi->setText(Themes->at(i));
        lwi->setIcon(QIcon(QString("%1/Themes/%2/icon.png").arg(datadir->absolutePath()).arg(Themes->at(i))));
        //lwi->setTextAlignment(Qt::AlignHCenter);
        lwThemes->addItem(lwi);
    }
    connect(lwThemes, SIGNAL(currentRowChanged(int)), this, SLOT(themeSelected(int)));

    // override default style to tighten up theme scroller
    lwThemes->setStyleSheet(QString(
        "QListWidget{"
            "border: solid;"
            "border-width: 0px;"
            "border-radius: 0px;"
            "border-color: transparent;"
            "background-color: #0d0544;"
            "color: #ffcc00;"
            "font: bold 13px;"
            "}"
        )
    );

    gbTLayout->addWidget(lwThemes);
    lwThemes->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    mapLayout->setSizeConstraint(QLayout::SetFixedSize);

    QWidget* seedWidget = new QWidget(this);
    mainLayout.addWidget(seedWidget, 1, 0);

    QGridLayout* seedLayout = new QGridLayout(seedWidget);
    seedLayout->setMargin(0);

    seedLabel = new QLabel(tr("Seed"), seedWidget);
    seedLayout->addWidget(seedLabel, 3, 0);
    seedEdit = new QLineEdit(seedWidget);
    seedEdit->setMaxLength(54);
    connect(seedEdit, SIGNAL(returnPressed()), this, SLOT(seedEdited()));
    seedLayout->addWidget(seedEdit, 3, 1);
    seedLayout->setColumnStretch(1, 5);
    seedSet = new QPushButton(seedWidget);
    seedSet->setText(QPushButton::tr("more"));
    connect(seedSet, SIGNAL(clicked()), this, SLOT(seedEdited()));
    seedLayout->setColumnStretch(2, 1);
    seedLayout->addWidget(seedSet, 3, 2);

    seedLabel->setVisible(false);
    seedEdit->setVisible(false);

    setRandomSeed();
    setRandomTheme();
}

void HWMapContainer::setImage(const QImage newImage)
{
    QPixmap px(256, 128);
    QPixmap pxres(256, 128);
    QPainter p(&pxres);

    px.fill(Qt::yellow);
    QBitmap bm = QBitmap::fromImage(newImage);
    px.setMask(bm);

    QLinearGradient linearGrad(QPoint(128, 0), QPoint(128, 128));
    linearGrad.setColorAt(1, QColor(0, 0, 192));
    linearGrad.setColorAt(0, QColor(66, 115, 225));
    p.fillRect(QRect(0, 0, 256, 128), linearGrad);
    p.drawPixmap(QPoint(0, 0), px);

    addInfoToPreview(pxres);
    //chooseMap->setCurrentIndex(mapgen);
    pMap = 0;
}

void HWMapContainer::setHHLimit(int newHHLimit)
{
    hhLimit = newHHLimit;
}

void HWMapContainer::mapChanged(int index)
{
    switch(index) {
    case MAPGEN_REGULAR:
        mapgen = MAPGEN_REGULAR;
        updatePreview();
        gbThemes->show();
        lblFilter->show();
        cbTemplateFilter->show();
        maze_size_label->hide();
        cbMazeSize->hide();
        emit mapChanged("+rnd+");
        emit mapgenChanged(mapgen);
        emit themeChanged(chooseMap->itemData(index).toList()[1].toString());
        break;
    case MAPGEN_MAZE:
        mapgen = MAPGEN_MAZE;
        updatePreview();
        gbThemes->show();
        lblFilter->hide();
        cbTemplateFilter->hide();
        maze_size_label->show();
        cbMazeSize->show();
        emit mapChanged("+maze+");
        emit mapgenChanged(mapgen);
        emit themeChanged(chooseMap->itemData(index).toList()[1].toString());
        break;
    case MAPGEN_DRAWN:
        mapgen = MAPGEN_DRAWN;
        updatePreview();
        gbThemes->show();
        lblFilter->hide();
        cbTemplateFilter->hide();
        maze_size_label->hide();
        cbMazeSize->hide();
        emit mapChanged("+drawn+");
        emit mapgenChanged(mapgen);
        emit themeChanged(chooseMap->itemData(index).toList()[1].toString());
        break;
    default:
        updatePreview();
        gbThemes->hide();
        lblFilter->hide();
        cbTemplateFilter->hide();
        maze_size_label->hide();
        cbMazeSize->hide();
        emit mapChanged(chooseMap->itemData(index).toList()[0].toString());
    }
}

// Should this add text to identify map size?
void HWMapContainer::addInfoToPreview(QPixmap image)
{
    QPixmap finalImage = QPixmap(image.size());
    finalImage.fill(QColor(0, 0, 0, 0));

    QPainter p(&finalImage);
    p.drawPixmap(image.rect(), image);
    //p.setPen(QColor(0xf4,0x9e,0xe9));
    p.setPen(QColor(0xff,0xcc,0x00));
    p.setBrush(QColor(0, 0, 0));
    p.drawRect(image.rect().width() - hhSmall.rect().width() - 28, 3, 40, 20);
    p.setFont(QFont("MS Shell Dlg", 10));
    p.drawText(image.rect().width() - hhSmall.rect().width() - 14 - (hhLimit > 9 ? 10 : 0), 18, QString::number(hhLimit));
    p.drawPixmap(image.rect().width() - hhSmall.rect().width() - 5, 5, hhSmall.rect().width(), hhSmall.rect().height(), hhSmall);

    imageButt->setIcon(finalImage);
    imageButt->setIconSize(image.size());
}

void HWMapContainer::askForGeneratedPreview()
{
    if (pMap)
    {
        disconnect(pMap, 0, this, SLOT(setImage(const QImage)));
        disconnect(pMap, 0, this, SLOT(setHHLimit(int)));
        pMap = 0;
    }

    pMap = new HWMap();
    connect(pMap, SIGNAL(ImageReceived(const QImage)), this, SLOT(setImage(const QImage)));
    connect(pMap, SIGNAL(HHLimitReceived(int)), this, SLOT(setHHLimit(int)));
    pMap->getImage(m_seed,
                   getTemplateFilter(),
                   get_mapgen(),
                   getMazeSize(),
                   getDrawnMapData()
            );
}

void HWMapContainer::themeSelected(int currentRow)
{
    QString theme = Themes->at(currentRow);
    QList<QVariant> mapInfo;
    mapInfo.push_back(QString("+rnd+"));
    mapInfo.push_back(theme);
    mapInfo.push_back(18);
    mapInfo.push_back(false);
    chooseMap->setItemData(0, mapInfo);
    mapInfo[0] = QString("+maze+");
    chooseMap->setItemData(1, mapInfo);
    mapInfo[0] = QString("+drawn+");
    chooseMap->setItemData(2, mapInfo);
    gbThemes->setIcon(QIcon(QString("%1/Themes/%2/icon@2x.png").arg(datadir->absolutePath()).arg(theme)));
    emit themeChanged(theme);
}

QString HWMapContainer::getCurrentSeed() const
{
    return m_seed;
}

QString HWMapContainer::getCurrentMap() const
{
    if(chooseMap->currentIndex() < MAPGEN_MAP) return QString();
    return chooseMap->itemData(chooseMap->currentIndex()).toList()[0].toString();
}

QString HWMapContainer::getCurrentTheme() const
{
    return chooseMap->itemData(chooseMap->currentIndex()).toList()[1].toString();
}

bool HWMapContainer::getCurrentIsMission() const
{
    if(!chooseMap->currentIndex()) return false;
    return chooseMap->itemData(chooseMap->currentIndex()).toList()[3].toBool();
}

int HWMapContainer::getCurrentHHLimit() const
{
    return hhLimit;
}

QString HWMapContainer::getCurrentScheme() const
{
    return chooseMap->itemData(chooseMap->currentIndex()).toList()[4].toString();
}

QString HWMapContainer::getCurrentWeapons() const
{
    return chooseMap->itemData(chooseMap->currentIndex()).toList()[5].toString();
}

quint32 HWMapContainer::getTemplateFilter() const
{
    return cbTemplateFilter->itemData(cbTemplateFilter->currentIndex()).toInt();
}

void HWMapContainer::resizeEvent ( QResizeEvent * event )
{
    Q_UNUSED(event);
  //imageButt->setIconSize(imageButt->size());
}

void HWMapContainer::intSetSeed(const QString & seed)
{
    m_seed = seed;
    if (seed != seedEdit->text())
        seedEdit->setText(seed);
}

void HWMapContainer::setSeed(const QString & seed)
{
    intSetSeed(seed);
    if (chooseMap->currentIndex() < MAPGEN_MAP)
        updatePreview();
}

void HWMapContainer::intSetMap(const QString & map)
{
    int id = 0;
    for(int i = 0; i < chooseMap->count(); i++)
        if(!chooseMap->itemData(i).isNull() && chooseMap->itemData(i).toList()[0].toString() == map)
        {
            id = i;
            break;
        }

    if(id > 0) {
        if (pMap)
        {
            disconnect(pMap, 0, this, SLOT(setImage(const QImage)));
            disconnect(pMap, 0, this, SLOT(setHHLimit(int)));
            pMap = 0;
        }
        chooseMap->setCurrentIndex(id);
    }
}

void HWMapContainer::setMap(const QString &map)
{
    intSetMap(map);
    updatePreview();
}

void HWMapContainer::setTheme(const QString & theme)
{
    QList<QListWidgetItem *> items = lwThemes->findItems(theme, Qt::MatchExactly);
    if(items.size())
        lwThemes->setCurrentItem(items.at(0));
}

void HWMapContainer::setRandomMap()
{
    setRandomSeed();
    switch(chooseMap->currentIndex())
    {
    case MAPGEN_REGULAR:
    case MAPGEN_MAZE:
        setRandomTheme();
        break;
    case MAPGEN_DRAWN:
        emit drawMapRequested();
        break;
    default:
        if(chooseMap->currentIndex() <= numMissions + MAPGEN_MAP + 1)
            setRandomMission();
        else
            setRandomStatic();
        break;
    }
}

void HWMapContainer::setRandomStatic()
{
    int i = MAPGEN_MAP + 3 + numMissions + rand() % (chooseMap->count() - MAPGEN_MAP - 3 - numMissions);
    chooseMap->setCurrentIndex(i);
    setRandomSeed();
}

void HWMapContainer::setRandomMission()
{
    int i = MAPGEN_MAP + 2 + rand() % numMissions;
    qDebug() << i << MAPGEN_MAP << numMissions;
    chooseMap->setCurrentIndex(i);
    setRandomSeed();
}

void HWMapContainer::setRandomSeed()
{
    m_seed = QUuid::createUuid().toString();
    seedEdit->setText(m_seed);
    emit seedChanged(m_seed);
    if (chooseMap->currentIndex() < MAPGEN_MAP)
        updatePreview();
}

void HWMapContainer::setRandomTheme()
{
    if(!Themes->size()) return;
    quint32 themeNum = rand() % Themes->size();
    lwThemes->setCurrentRow(themeNum);
}

void HWMapContainer::intSetTemplateFilter(int filter)
{
    cbTemplateFilter->setCurrentIndex(filter);
    emit newTemplateFilter(filter);
}

void HWMapContainer::setTemplateFilter(int filter)
{
    intSetTemplateFilter(filter);
    updatePreview();
}

MapGenerator HWMapContainer::get_mapgen(void) const
{
    return mapgen;
}

int HWMapContainer::getMazeSize(void) const
{
    return cbMazeSize->currentIndex();
}

void HWMapContainer::intSetMazeSize(int size)
{
    cbMazeSize->setCurrentIndex(size);
    emit mazeSizeChanged(size);
}

void HWMapContainer::setMazeSize(int size)
{
    intSetMazeSize(size);
    updatePreview();
}

void HWMapContainer::intSetMapgen(MapGenerator m)
{
    mapgen = m;
    chooseMap->setCurrentIndex(m);
    emit mapgenChanged(m);
}

void HWMapContainer::setMapgen(MapGenerator m)
{
    intSetMapgen(m);
    updatePreview();
}

void HWMapContainer::setDrawnMapData(const QByteArray & ar)
{
    drawMapScene.decode(ar);
    updatePreview();
}

QByteArray HWMapContainer::getDrawnMapData()
{
    return drawMapScene.encode();
}

void HWMapContainer::seedEdited()
{
    if (seedLabel->isVisible() == false )
    {
        seedLabel->setVisible(true);
        seedEdit->setVisible(true);
        seedSet->setText(tr("Set"));
        return;
    }

    if (seedEdit->text().isEmpty())
        seedEdit->setText(m_seed);
    else
    {
        setSeed(seedEdit->text());
        emit seedChanged(seedEdit->text());
    }
}

DrawMapScene * HWMapContainer::getDrawMapScene()
{
    return &drawMapScene;
}

void HWMapContainer::mapDrawingFinished()
{
    emit drawnMapChanged(getDrawnMapData());

    updatePreview();
}

void HWMapContainer::updatePreview()
{
    int curIndex = chooseMap->currentIndex();

    switch(curIndex)
    {
    case MAPGEN_REGULAR:
        askForGeneratedPreview();
        break;
    case MAPGEN_MAZE:
        askForGeneratedPreview();
        break;
    case MAPGEN_DRAWN:
        askForGeneratedPreview();
        break;
    default:
        QPixmap mapImage;
        qDebug() << "Map data" << curIndex << chooseMap->currentText() << chooseMap->itemData(curIndex);
        if(!mapImage.load(datadir->absolutePath() + "/Maps/" + chooseMap->itemData(curIndex).toList()[0].toString() + "/preview.png")) {
            imageButt->setIcon(QIcon());
            return;
        }

        hhLimit = chooseMap->itemData(curIndex).toList()[2].toInt();
        addInfoToPreview(mapImage);
    }
}

void HWMapContainer::setAllMapParameters(const QString &map, MapGenerator m, int mazesize, const QString &seed, int tmpl)
{
    intSetMap(map);
    intSetMapgen(m);
    intSetMazeSize(mazesize);
    intSetSeed(seed);
    intSetTemplateFilter(tmpl);

    updatePreview();
}
