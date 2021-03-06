/*
 * Hedgewars, a free turn based strategy game
 * Copyright (c) 2006-2011 Andrey Korotaev <unC0Rr@gmail.com>
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

#ifndef PAGES_H
#define PAGES_H

#include <QWidget>
#include <QPushButton>
#include <QFont>
#include <QGridLayout>
#include <QSignalMapper>

#include "binds.h"
#include "hwform.h"
#include "mapContainer.h"
#include "togglebutton.h"

class QPushButton;
class QGroupBox;
class QComboBox;
class QLabel;
class QToolBox;
class QLineEdit;
class QListWidget;
class QCheckBox;
class QSpinBox;
class QTextEdit;
class QRadioButton;
class QTableView;
class QTextBrowser;
class QTableWidget;
class QAction;
class QDataWidgetMapper;
class QAbstractItemModel;
class QSettings;
class QSlider;

class GameCFGWidget;
class TeamSelWidget;
class DemosList;
class SquareLabel;
class About;
class FPSEdit;
class HWChatWidget;
class SelWeaponWidget;
class IconedGroupBox;
class FreqSpinBox;

class DrawMapWidget;

class AbstractPage : public QWidget
{
    Q_OBJECT

 public:

 protected:
  AbstractPage(QWidget* parent = 0) {
    Q_UNUSED(parent);

    font14 = new QFont("MS Shell Dlg", 14);
    //setFocusPolicy(Qt::StrongFocus);
  }
  virtual ~AbstractPage() {};

  QPushButton* addButton(QString btname, QGridLayout* grid, int wy, int wx, bool iconed = false) {
    QPushButton* butt = new QPushButton(this);
    if (!iconed) {
      butt->setFont(*font14);
      butt->setText(btname);
      //butt->setStyleSheet("background-color: #0d0544");
    } else {
      const QIcon& lp=QIcon(btname);
      QSize sz = lp.actualSize(QSize(65535, 65535));
      butt->setIcon(lp);
      butt->setFixedSize(sz);
      butt->setIconSize(sz);
      butt->setFlat(true);
      butt->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    grid->addWidget(butt, wy, wx);
    return butt;
  };

  QPushButton* addButton(QString btname, QGridLayout* grid, int wy, int wx, int rowSpan, int columnSpan, bool iconed = false) {
    QPushButton* butt = new QPushButton(this);
    if (!iconed) {
      butt->setFont(*font14);
      butt->setText(btname);
    } else {
      const QIcon& lp=QIcon(btname);
      QSize sz = lp.actualSize(QSize(65535, 65535));
      butt->setIcon(lp);
      butt->setFixedSize(sz);
      butt->setIconSize(sz);
      butt->setFlat(true);
      butt->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    grid->addWidget(butt, wy, wx, rowSpan, columnSpan);
    return butt;
  };

  QPushButton* addButton(QString btname, QBoxLayout* box, int where, bool iconed = false) {
    QPushButton* butt = new QPushButton(this);
    if (!iconed) {
      butt->setFont(*font14);
      butt->setText(btname);
    } else {
      const QIcon& lp=QIcon(btname);
      QSize sz = lp.actualSize(QSize(65535, 65535));
      butt->setIcon(lp);
      butt->setFixedSize(sz);
      butt->setIconSize(sz);
      butt->setFlat(true);
      butt->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    box->addWidget(butt, where);
    return butt;
  };

  QFont * font14;
};

class PageMain : public AbstractPage
{
    Q_OBJECT

public:
    PageMain(QWidget* parent = 0);

    QPushButton *BtnSinglePlayer;
    QPushButton *BtnNet;
    QPushButton *BtnSetup;
    QPushButton *BtnInfo;
    QPushButton *BtnExit;
    QLabel *mainNote;
};

class PageEditTeam : public AbstractPage
{
    Q_OBJECT

public:
    PageEditTeam(QWidget* parent, SDLInteraction * sdli);
    QSignalMapper* signalMapper;
    QGroupBox *GBoxHedgehogs;
    QGroupBox *GBoxTeam;
    QGroupBox *GBoxFort;
    QComboBox *CBFort;
    SquareLabel *FortPreview;
    QComboBox *CBGrave;
    QComboBox *CBFlag;
    QComboBox *CBTeamLvl;
    QComboBox *CBVoicepack;
    QGroupBox *GBoxBinds;
    QToolBox *BindsBox;
    QPushButton *BtnTeamDiscard;
    QPushButton *BtnTeamSave;
    QPushButton * BtnTestSound;
    QLineEdit * TeamNameEdit;
    QLineEdit * HHNameEdit[8];
    QComboBox * HHHats[8];
    QPushButton * randButton[8];
    QComboBox * CBBind[BINDS_NUMBER];
    QPushButton * randTeamButton;

private:
    SDLInteraction * mySdli;

public slots:
    void CBFort_activated(const QString & gravename);

private slots:
    void testSound();
};

class PageMultiplayer : public AbstractPage
{
    Q_OBJECT

public:
    PageMultiplayer(QWidget* parent = 0);

    QPushButton *BtnBack;
    GameCFGWidget *gameCFG;
    TeamSelWidget *teamsSelect;
    QPushButton *BtnStartMPGame;

signals:
    void SetupClicked();
};

class PageOptions : public AbstractPage
{
    Q_OBJECT

public:
    PageOptions(QWidget* parent = 0);

    QCheckBox *WeaponTooltip;
    QPushButton *WeaponNew;
    QPushButton *WeaponEdit;
    QPushButton *WeaponDelete;
    QComboBox *WeaponsName;
    QPushButton *SchemeNew;
    QPushButton *SchemeEdit;
    QPushButton *SchemeDelete;
    QComboBox *SchemesName;

    QComboBox *CBLanguage;

    QPushButton *BtnBack;
    IconedGroupBox *teamsBox;
    QPushButton *BtnNewTeam;
    QPushButton *BtnEditTeam;
    QPushButton *BtnDeleteTeam;
    QPushButton *BtnAssociateFiles;
    QLabel *LblNoEditTeam;
    QComboBox *CBTeamName;
    IconedGroupBox *AGGroupBox;
    QComboBox *CBResolution;
    QComboBox *CBStereoMode;
    QCheckBox *CBEnableSound;
    QCheckBox *CBEnableFrontendSound;
    QCheckBox *CBEnableMusic;
    QCheckBox *CBEnableFrontendMusic;
    QCheckBox *CBFullscreen;
    QCheckBox *CBFrontendFullscreen;
    QCheckBox *CBShowFPS;
    QCheckBox *CBAltDamage;
    QCheckBox *CBNameWithDate;
#ifdef __APPLE__
    QCheckBox *CBAutoUpdate;
#endif

    FPSEdit *fpsedit;
    QPushButton *BtnSaveOptions;
    QLabel *labelNN;
    QSpinBox * volumeBox;
    QLineEdit *editNetNick;
    QSlider *SLQuality;
    QCheckBox *CBFrontendEffects;

private:
    bool previousFullscreenValue;
    int previousResolutionIndex;
    int previousQuality;

private slots:
    void forceFullscreen(int index);
    void setFullscreen(void);
    void trimNetNick();
};

class PageNet : public AbstractPage
{
    Q_OBJECT

public:
    PageNet(QWidget* parent = 0);

    QPushButton* BtnUpdateSList;
    QTableView * tvServersList;
    QPushButton * BtnBack;
    QPushButton * BtnNetConnect;
    QPushButton * BtnNetSvrStart;
    QPushButton * BtnSpecifyServer;

private:
    QGroupBox * ConnGroupBox;
    QGridLayout * GBClayout;

private slots:
    void slotConnect();

public slots:
    void updateServersList();

signals:
    void connectClicked(const QString & host, quint16 port);
};

class PageNetServer : public AbstractPage
{
    Q_OBJECT

public:
    PageNetServer(QWidget* parent = 0);

    QPushButton *BtnBack;
    QPushButton *BtnStart;
    QPushButton *BtnDefault;
    QLabel *labelSD;
    QLineEdit *leServerDescr;
    QLabel *labelPort;
    QSpinBox *sbPort;

private slots:
    void setDefaultPort();
};

class PageNetGame : public AbstractPage
{
    Q_OBJECT

public:
    PageNetGame(QWidget* parent, QSettings * config, SDLInteraction * sdli);

    QPushButton *BtnBack;
    QPushButton *BtnGo;
    QPushButton *BtnMaster;
    QPushButton *BtnStart;

    QAction * restrictJoins;
    QAction * restrictTeamAdds;

    HWChatWidget* pChatWidget;

    TeamSelWidget* pNetTeamsWidget;
    GameCFGWidget* pGameCFG;

public slots:
    void setReadyStatus(bool isReady);
    void setMasterMode(bool isMaster);

signals:
    void SetupClicked();
};

class PageInfo : public AbstractPage
{
    Q_OBJECT

public:
    PageInfo(QWidget* parent = 0);

    QPushButton *BtnBack;
    About *about;
};

class PageSinglePlayer : public AbstractPage
{
    Q_OBJECT

public:
    PageSinglePlayer(QWidget* parent = 0);

    QPushButton *BtnSimpleGamePage;
    QPushButton *BtnTrainPage;
    QPushButton *BtnCampaignPage;
    QPushButton *BtnMultiplayer;
    QPushButton *BtnLoad;
    QPushButton *BtnDemos;
    QPushButton *BtnBack;
    GameCFGWidget *gameCFG;
};

class PageTraining : public AbstractPage
{
    Q_OBJECT

public:
    PageTraining(QWidget* parent = 0);

    QPushButton *BtnStartTrain;
    QPushButton *BtnBack;
    QComboBox   *CBSelect;
};

class PageCampaign : public AbstractPage
{
    Q_OBJECT

public:
    PageCampaign(QWidget* parent = 0);

    QPushButton *BtnStartCampaign;
    QPushButton *BtnBack;
    QComboBox   *CBSelect;
    QComboBox   *CBTeam;
};

class PageSelectWeapon : public AbstractPage
{
    Q_OBJECT

public:
    PageSelectWeapon(QWidget* parent = 0);

    QPushButton *BtnSave;
    QPushButton *BtnDefault;
    QPushButton *BtnDelete;
    QPushButton *BtnNew;
    QPushButton *BtnCopy;
    QPushButton *BtnBack;
    SelWeaponWidget* pWeapons;
    QComboBox* selectWeaponSet;
};

class PageInGame : public AbstractPage
{
    Q_OBJECT

public:
    PageInGame(QWidget* parent = 0);
};

class PageRoomsList : public AbstractPage
{
    Q_OBJECT

public:
    PageRoomsList(QWidget* parent, QSettings * config, SDLInteraction * sdli);

    QLineEdit * roomName;
    QLineEdit * searchText;
    QTableWidget * roomsList;
    QPushButton * BtnBack;
    QPushButton * BtnCreate;
    QPushButton * BtnJoin;
    QPushButton * BtnRefresh;
    QPushButton * BtnAdmin;
    QPushButton * BtnClear;
    QComboBox * CBState;
    QComboBox * CBRules;
    QComboBox * CBWeapons;
    HWChatWidget * chatWidget;

private:
    bool gameInLobby;
    QString gameInLobbyName;
    QStringList listFromServer;
    AmmoSchemeModel * ammoSchemeModel;

public slots:
    void setRoomsList(const QStringList & list);
    void setAdmin(bool);

private slots:
    void onCreateClick();
    void onJoinClick();
    void onRefreshClick();
    void onClearClick();
    void onJoinConfirmation(const QString &);

signals:
    void askForCreateRoom(const QString &);
    void askForJoinRoom(const QString &);
    void askForRoomList();
    void askJoinConfirmation(const QString &);
};

class PageConnecting : public AbstractPage
{
    Q_OBJECT

public:
    PageConnecting(QWidget* parent = 0);
};

class PageScheme : public AbstractPage
{
    Q_OBJECT

public:
    PageScheme(QWidget* parent = 0);

    QPushButton * BtnBack;
    QPushButton * BtnCopy;
    QPushButton * BtnNew;
    QPushButton * BtnDelete;
    QPushButton * BtnSave;
    QComboBox * selectScheme;

    void setModel(QAbstractItemModel * model);

public slots:
    void newRow();
    void copyRow();
    void deleteRow();

private:
    QDataWidgetMapper * mapper;
    ToggleButtonWidget * TBW_mode_Forts;
    ToggleButtonWidget * TBW_teamsDivide;
    ToggleButtonWidget * TBW_solid;
    ToggleButtonWidget * TBW_border;
    ToggleButtonWidget * TBW_lowGravity;
    ToggleButtonWidget * TBW_laserSight;
    ToggleButtonWidget * TBW_invulnerable;
    ToggleButtonWidget * TBW_resethealth;
    ToggleButtonWidget * TBW_vampiric;
    ToggleButtonWidget * TBW_karma;
    ToggleButtonWidget * TBW_artillery;
    ToggleButtonWidget * TBW_randomorder;
    ToggleButtonWidget * TBW_king;
    ToggleButtonWidget * TBW_placehog;
    ToggleButtonWidget * TBW_sharedammo;
    ToggleButtonWidget * TBW_disablegirders;
    ToggleButtonWidget * TBW_disablelandobjects;
    ToggleButtonWidget * TBW_aisurvival;
    ToggleButtonWidget * TBW_infattack;
    ToggleButtonWidget * TBW_resetweps;
    ToggleButtonWidget * TBW_perhogammo;
    ToggleButtonWidget * TBW_nowind;
    ToggleButtonWidget * TBW_morewind;
    ToggleButtonWidget * TBW_tagteam;

    QSpinBox * SB_DamageModifier;
    QSpinBox * SB_TurnTime;
    QSpinBox * SB_InitHealth;
    QSpinBox * SB_SuddenDeath;
    QSpinBox * SB_WaterRise;
    QSpinBox * SB_HealthDecrease;
    FreqSpinBox * SB_CaseProb;
    QSpinBox * SB_HealthCrates;
    QSpinBox * SB_CrateHealth;
    QSpinBox * SB_MinesTime;
    QSpinBox * SB_Mines;
    QSpinBox * SB_MineDuds;
    QSpinBox * SB_Explosives;
    QSpinBox * SB_RopeModifier;
    QLineEdit * LE_name;

    QGroupBox * gbGameModes;
    QGroupBox * gbBasicSettings;

private slots:
    void schemeSelected(int);

};

class PageAdmin : public AbstractPage
{
    Q_OBJECT

public:
    PageAdmin(QWidget* parent = 0);

    QPushButton * BtnBack;
    QPushButton * pbClearAccountsCache;

private:
    QLineEdit * leServerMessageNew;
    QLineEdit * leServerMessageOld;
    QPushButton * pbSetSM;
    QPushButton * pbAsk;
    QSpinBox * sbProtocol;
    QTextBrowser * tb;

private slots:
    void smChanged();

public slots:
    void serverMessageNew(const QString & str);
    void serverMessageOld(const QString & str);
    void protocol(int proto);

signals:
    void setServerMessageNew(const QString & str);
    void setServerMessageOld(const QString & str);
    void setProtocol(int proto);
    void askServerVars();
};


class PageNetType : public AbstractPage
{
    Q_OBJECT

public:
    PageNetType(QWidget* parent = 0);

    QPushButton * BtnBack;
    QPushButton * BtnLAN;
    QPushButton * BtnOfficialServer;
};


class PageDrawMap : public AbstractPage
{
    Q_OBJECT

public:
    PageDrawMap(QWidget* parent = 0);

    QPushButton * BtnBack;

    DrawMapWidget * drawMapWidget;

private slots:
    void load();
    void save();
};

#endif // PAGES_H
