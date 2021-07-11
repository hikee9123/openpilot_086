#pragma once



#include <QWidget>
#include <QFrame>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QButtonGroup>
#include <QScrollArea>
#include <QStackedWidget>

#include "selfdrive/ui/qt/widgets/controls.h"


class CUserPanel : public QFrame 
{
  Q_OBJECT
public:
  explicit CUserPanel(QWidget* parent = nullptr);


protected:
  void showEvent(QShowEvent *event) override;

};





class CAutoResumeToggle : public ToggleControl {
  Q_OBJECT

public:
  CAutoResumeToggle() : ToggleControl("자동출발 기능 사용", "SCC 사용중 정차시 자동출발 기능을 사용합니다.", "../assets/offroad/icon_shell.png", Params().getBool("OpkrAutoResume")) {
    QObject::connect(this, &CAutoResumeToggle::toggleFlipped, [=](int state) {
      Params().putBool("OpkrAutoResume", (bool)state);
    });
  }
};


class RunNaviOnBootToggle : public ToggleControl {
  Q_OBJECT

public:
  RunNaviOnBootToggle() : ToggleControl("부팅 후 네비 자동 실행", "부팅후 네비게이션(티맵)을 자동 실행합니다.", "../assets/offroad/icon_shell.png", Params().getBool("OpkrRunNaviOnBoot")) {
    QObject::connect(this, &RunNaviOnBootToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("OpkrRunNaviOnBoot", &value, 1);
    });
  }
};

class CLiveSteerRatioToggle : public AbstractControl {
  Q_OBJECT

public:
  CLiveSteerRatioToggle();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class CTurnSteeringDisableToggle : public ToggleControl {
  Q_OBJECT

public:
  CTurnSteeringDisableToggle() : ToggleControl("턴시그널 사용시 조향해제 사용", "차선변경속도 이하로 주행할 때 턴시그널을 사용시 자동조향을 일시해제 합니다.", "../assets/offroad/icon_shell.png", Params().getBool("OpkrTurnSteeringDisable")) {
    QObject::connect(this, &CTurnSteeringDisableToggle::toggleFlipped, [=](int state) {
      Params().putBool("OpkrTurnSteeringDisable", (bool)state);
    });
  }
};


class CPrebuiltToggle : public ToggleControl {
  Q_OBJECT

public:
  CPrebuiltToggle() : ToggleControl("Prebuilt 파일 생성", "Prebuilt 파일을 생성하며 부팅속도를 단축시킵니다. UI수정을 한 경우 기능을 끄십시오.", "../assets/offroad/icon_shell.png", Params().getBool("PutPrebuiltOn")) {
    QObject::connect(this, &CPrebuiltToggle::toggleFlipped, [=](int state) {
      Params().putBool("PutPrebuiltOn", (bool)state);
    });
  }
};


class CLongitudinalControlToggle : public ToggleControl {
  Q_OBJECT

public:
  CLongitudinalControlToggle() : ToggleControl("Longitudinal Control", "가감속 제어를 오픈파일럿에서 제어 합니다.", "../assets/offroad/icon_shell.png", Params().getBool("LongitudinalControl")) {
    QObject::connect(this, &CLongitudinalControlToggle::toggleFlipped, [=](int state) {
      Params().putBool("LongitudinalControl", (bool)state);
    });
  }
};

class BrightnessControl : public AbstractControl 
{
  Q_OBJECT

public:
  BrightnessControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class CVolumeControl : public AbstractControl {
  Q_OBJECT

public:
  CVolumeControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};


class AutoScreenOff : public AbstractControl {
  Q_OBJECT

public:
  AutoScreenOff();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class CAutoFocus : public AbstractControl {
  Q_OBJECT

public:
  CAutoFocus();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};
////////////////////////////////////////////////////////////////////////////////////////
//
//  Git

class GitHash : public AbstractControl {
  Q_OBJECT

public:
  GitHash();

private:
  QLabel local_hash;
  QLabel remote_hash;
};



class SshLegacyToggle : public ToggleControl {
  Q_OBJECT

public:
  SshLegacyToggle() : ToggleControl("기존 공개KEY 사용", "SSH 접속시 기존 공개KEY(0.8.2이하)를 사용합니다.", "", Params().getBool("OpkrSSHLegacy")) {
    QObject::connect(this, &SshLegacyToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("OpkrSSHLegacy", &value, 1);
    });
  }
};


////////////////////////////////////////////////////////////////////////////////////////
//
//  Combo box
class CarSelectCombo : public AbstractControl 
{
  Q_OBJECT

public:
  CarSelectCombo();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  QComboBox  combobox;

  void refresh();

};
