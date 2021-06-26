
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>



#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/qt/widgets/toggle.h"
#include "selfdrive/ui/qt/widgets/offroad_alerts.h"
#include "selfdrive/ui/qt/widgets/controls.h"

#include "selfdrive/common/params.h"
#include "selfdrive/common/util.h"
#include "selfdrive/ui/ui.h"

#include "userPanel.hpp"

// 일부 코드 OPKR 참고.
/*
def exec_app(status, app, app_main):
  if status == "1":
    std::system("pm enable %s" % app)
    std::system("am start -n %s/%s" % (app, app_main))
    return True
  if status == "0":
    std::system("pm disable %s" % app)
    return False
*/

CUserPanel::CUserPanel(QWidget* parent) : QFrame(parent)
{

  //  UIState* ui_state = &QUIState::ui_state;
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setMargin(100);
  setLayout(main_layout);

  auto exe_git_pull_reboot = new ButtonControl("* PROGRAM DOWNLOAD<reboot>", "실행",
                                        "리모트 Git에서 변경사항이 있으면 로컬에 반영 후 자동 재부팅 됩니다. 변경사항이 없으면 재부팅하지 않습니다. 로컬 파일이 변경된경우 리모트Git 내역을 반영 못할수도 있습니다. 참고바랍니다.");
  connect(exe_git_pull_reboot, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to git pull?")) 
          {
            const char* gitpull = "/data/openpilot/gitpull.sh ''";            
            std::system(gitpull);
          }
  });


  auto exe_git_pull = new ButtonControl("Git Pull 실행", "실행",
                                    "리모트 Git에서 변경사항이 있으면 로컬에 반영 됩니다. 로컬 파일이 변경된경우 리모트Git 내역을 반영 못할수도 있습니다. 참고바랍니다.");
  connect(exe_git_pull, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to git pull?")) 
          {
            std::system("git pull");
          }
  });

  auto exe_git_cancel = new ButtonControl("Git Pull 취소", "실행", 
                                    "Git Pull을 취소하고 이전상태로 되돌립니다. 커밋내역이 여러개인경우 최신커밋 바로 이전상태로 되돌립니다.");
  connect(exe_git_cancel, &ButtonControl::released, [=]() 
  { 
        if (ConfirmationDialog::confirm("GitPull 이전 상태로 되돌립니다. 진행하시겠습니까?")){
          const char* gitpull_cancel = "/data/openpilot/gitpull_cancel.sh ''";
          std::system(gitpull_cancel);
        }
  });  

   

  for (auto btn : { exe_git_pull_reboot, exe_git_pull, exe_git_cancel}) {
    if (btn) {
      //layout()->addWidget(horizontal_line());
      connect(parent, SIGNAL(offroadTransition(bool)), btn, SLOT(setEnabled(bool)));
      layout()->addWidget(btn);
    }
  }  

  layout()->addWidget(horizontal_line());  
  layout()->addWidget(new GitHash());

  layout()->addWidget(horizontal_line());
  
  layout()->addWidget(new SshLegacyToggle());

  layout()->addWidget(horizontal_line());


  layout()->addWidget(new ParamControl("IsOpenpilotViewEnabled",
                                  "주행화면 미리보기",
                                  "오픈파일럿 주행화면을 미리보기 합니다.",
                                  "../assets/offroad/icon_network.png",
                                  this));


  layout()->addWidget(horizontal_line());

   layout()->addWidget(new CAutoResumeToggle());
   layout()->addWidget(new CLiveSteerRatioToggle());
   layout()->addWidget(new CTurnSteeringDisableToggle());
   layout()->addWidget(new CPrebuiltToggle());
  
  layout()->addWidget(horizontal_line());
  layout()->addWidget(new CLongitudinalControlToggle() );
  layout()->addWidget(horizontal_line());  
  layout()->addWidget(new RunNaviOnBootToggle());
  layout()->addWidget(horizontal_line());

  layout()->addWidget(new BrightnessControl());
  layout()->addWidget(new CVolumeControl());  
  layout()->addWidget(new AutoScreenOff());
  layout()->addWidget(new CAutoFocus());

  layout()->addWidget(horizontal_line());


  auto car_interfaces = new ButtonControl("car interfaces 실행", "실행",
                                        "/data/openpilot/selfdrive/car/tests/test_car_interfaces.py 을 실행 합니다.");
  connect(car_interfaces, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(test_car_interfaces.py)?")) 
          {
            std::system("python /data/openpilot/selfdrive/car/tests/test_car_interfaces.py");
          }
  });

  auto build_exe = new ButtonControl("build 실행", "실행",
                                        "/data/openpilot/selfdrive/manager/build.py 을 실행 합니다.");
  connect(build_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(build.py)?")) 
          {
            std::system("python /data/openpilot/selfdrive/manager/build.py");
          }
  });


  auto finger_exe = new ButtonControl("핑거 2.0 실행", "실행",
                                        "/data/openpilot/selfdrive/car/fw_versions.py 을 실행 합니다.");
  connect(finger_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(fw_versions.py)?")) 
          {
            std::system("python /data/openpilot/selfdrive/car/fw_versions.py");
          }
  });

  auto android_exe = new ButtonControl("Android Open", "Open",
                                        "Android를 Open 합니다.");
  connect(android_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(Android)?")) 
          {
            std::system("am start -a android.settings.SETTINGS");
          }
  });

  auto apk_exe = new ButtonControl("apk.py Install", "Install",
                                        "/data/openpilot/selfdrive/assets/addon/apk/apk.py 을 실행 합니다.");
  connect(apk_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(apk.py)?")) 
          {
            std::system("python /data/openpilot/selfdrive/assets/addon/apk/apk.py");
          }
  });

  auto mixplorer_exe = new ButtonControl("com.mixplorer Open", "Open",
                                   "run_mixplorer.sh 을 실행 합니다.");
  connect(mixplorer_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(com.mixplorer)?")) 
          {
            //std::system("am start -n com.mixplorer/com.mixplorer.activities.BrowseActivity");
            std::system("/data/openpilot/run_mixplorer.sh");
            
          }
  });

   auto tmap_exe = new ButtonControl("Tmap Open", "Open",
                                   "Tmap 을 실행 합니다.");
  connect(tmap_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(Tmap)?")) 
          {
            std::system("am start com.skt.tmap.ku/com.skt.tmap.activity.TmapNaviActivity &");

            //QProcess::execute("am start com.skt.tmap.ku/com.skt.tmap.activity.TmapNaviActivity");
            //QProcess::execute("pkill com.skt.tmap.ku");

            //QProcess::execute("am start --activity-task-on-home com.opkr.maphack/com.opkr.maphack.MainActivity");  // OPKR add map
          }
  });

   auto softkey_exe = new ButtonControl("Soft Key Open", "Open",
                                   "Soft Key 을 실행 합니다.");
  connect(softkey_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(Soft Key)?")) 
          {
            //std::system("pm enable com.gmd.hidesoftkeys")
            std::system("am start com.gmd.hidesoftkeys/com.gmd.hidesoftkeys.MainActivity");
          }
  });  

  for (auto btn : {car_interfaces, build_exe, finger_exe, android_exe, apk_exe, mixplorer_exe, tmap_exe}) {
    if (btn) {
      layout()->addWidget(horizontal_line());
      connect(parent, SIGNAL(offroadTransition(bool)), btn, SLOT(setEnabled(bool)));
      layout()->addWidget(btn);
    }
  }

  layout()->addWidget(horizontal_line());
  layout()->addWidget( new CarSelectCombo() );
}

void CUserPanel::showEvent(QShowEvent *event) 
{
  Params params = Params();


}



////////////////////////////////////////////////////////////////////////////////////////
//
//  CLiveSteerRatioToggle

CLiveSteerRatioToggle::CLiveSteerRatioToggle() : AbstractControl("Live SteerRatio 사용", "가변/고정 SR 대신 Live SteerRatio를 사용합니다. 반학습:SteerRatio값 학습.", "../assets/offroad/icon_shell.png") 
{
  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrLiveSteerRatio"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 0 ) {
      value = 0;
    }

    QString values = QString::number(value);
    Params().put("OpkrLiveSteerRatio", values.toStdString());
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrLiveSteerRatio"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 2 ) {
      value = 2;
    }

    QString values = QString::number(value);
    Params().put("OpkrLiveSteerRatio", values.toStdString());
    refresh();
  });
  refresh();
}

void CLiveSteerRatioToggle::refresh() 
{
  QString option = QString::fromStdString(Params().get("OpkrLiveSteerRatio"));
  if (option == "0") 
  {
    label.setText(QString::fromStdString("수동"));
  } 
  else if (option == "1") 
  {
    label.setText(QString::fromStdString("학습"));
  }
  else if (option == "2") 
  {
    label.setText(QString::fromStdString("반학습"));
  } 
  else 
  {
    label.setText( option );
  }
  btnminus.setText("－");
  btnplus.setText("＋");
}


////////////////////////////////////////////////////////////////////////////////////////
//
//  BrightnessControl


BrightnessControl::BrightnessControl() : AbstractControl("EON 밝기 조절(%)", "EON화면의 밝기를 조절합니다.", "../assets/offroad/icon_shell.png") 
{
  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrUIBrightness"));
    int value = str.toInt();
    value = value - 5;
    if (value <= 0 ) {
      value = 0;
    } else {
    }

    QUIState::ui_state.scene.scr.brightness = value;
    QString values = QString::number(value);
    Params().put("OpkrUIBrightness", values.toStdString());
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrUIBrightness"));
    int value = str.toInt();
    value = value + 5;
    if (value >= 100 ) {
      value = 100;
    } else {
    }

    QUIState::ui_state.scene.scr.brightness = value;
    QString values = QString::number(value);
    Params().put("OpkrUIBrightness", values.toStdString());
    refresh();
  });
  refresh();
}

void BrightnessControl::refresh() 
{
  QString option = QString::fromStdString(Params().get("OpkrUIBrightness"));
  if (option == "0") {
    label.setText(QString::fromStdString("자동조절"));
  } else {
    label.setText(QString::fromStdString(Params().get("OpkrUIBrightness")));
  }
  btnminus.setText("－");
  btnplus.setText("＋");
}


CVolumeControl::CVolumeControl() : AbstractControl("EON 볼륨 조절(%)", "EON의 볼륨을 조절합니다. 안드로이드 기본값/수동설정", "../assets/offroad/icon_shell.png") {

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrUIVolumeBoost"));
    int value = str.toInt();
    value = value - 10;
    if (value < 0 ) {
      value = 0;
    } 
    QString values = QString::number(value);
    QUIState::ui_state.scene.scr.nVolumeBoost = value;
    Params().put("OpkrUIVolumeBoost", values.toStdString());
    refresh();
   // QUIState::ui_state.sound->volume = value * 0.005;
   // QUIState::ui_state.sound->play(AudibleAlert::CHIME_WARNING1);
  });
  
  QObject::connect(&btnplus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrUIVolumeBoost"));
    int value = str.toInt();
    value = value + 10;
    if (value > 100 ) {
      value = 100;
    } 
    QString values = QString::number(value);
    QUIState::ui_state.scene.scr.nVolumeBoost = value;
    Params().put("OpkrUIVolumeBoost", values.toStdString());
    refresh();
  //  QUIState::ui_state.sound->volume = value * 0.005;
   // QUIState::ui_state.sound->play(AudibleAlert::CHIME_WARNING1);
  });
  refresh();
}

void CVolumeControl::refresh() {
  QString option = QString::fromStdString(Params().get("OpkrUIVolumeBoost"));
  if (option == "0") {
    label.setText(QString::fromStdString("기본값"));
  } else {
    label.setText(QString::fromStdString(Params().get("OpkrUIVolumeBoost")));
  }
  btnminus.setText("－");
  btnplus.setText("＋");
}

////////////////////////////////////////////////////////////////////////////////////////
//
//  AutoScreenOff


AutoScreenOff::AutoScreenOff() : AbstractControl("EON 화면 끄기(분)", "주행 시작 후 화면보호를 위해 이온화면이 꺼지는 시간을 설정합니다. 터치나 이벤트 발생시 자동으로 켜집니다.", "../assets/offroad/icon_shell.png") 
{

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrAutoScreenOff"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 0 ) {
      value = 0;
    } else {
    }

    QUIState::ui_state.scene.scr.autoScreenOff = value;
    QString values = QString::number(value);
    Params().put("OpkrAutoScreenOff", values.toStdString());
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrAutoScreenOff"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 10 ) {
      value = 10;
    } else {
    }

    QUIState::ui_state.scene.scr.autoScreenOff = value;
    QString values = QString::number(value);
    Params().put("OpkrAutoScreenOff", values.toStdString());
    refresh();
  });
  refresh();
}

void AutoScreenOff::refresh() 
{
  QString option = QString::fromStdString(Params().get("OpkrAutoScreenOff"));
  if (option == "0") {
    label.setText(QString::fromStdString("항상켜기"));
  } else {
    label.setText(QString::fromStdString(Params().get("OpkrAutoScreenOff")));
  }
  btnminus.setText("－");
  btnplus.setText("＋");
}


////////////////////////////////////////////////////////////////////////////////////////
//
//  Auto Focus

  

CAutoFocus::CAutoFocus() : AbstractControl("Auto Focus", "Focus을 변경합니다.  0:auto  1~100:manaul", "../assets/offroad/icon_shell.png") 
{

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrAutoFocus"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 0 ) {
      value = 0;
    } else {
    }

    Params::param_value.autoFocus = value;
    QString values = QString::number(value);
    Params().put("OpkrAutoFocus", values.toStdString());
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::released, [=]() {
    auto str = QString::fromStdString(Params().get("OpkrAutoFocus"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 100 ) {
      value = 100;
    } else {
    }

    Params::param_value.autoFocus = value;
    QString values = QString::number(value);
    Params().put("OpkrAutoFocus", values.toStdString());
    refresh();
  });
  refresh();
}

void CAutoFocus::refresh() 
{
  QString option = QString::fromStdString(Params().get("OpkrAutoFocus"));
  if (option == "0") {
    label.setText(QString::fromStdString("자동"));
  } else {
    label.setText(QString::fromStdString(Params().get("OpkrAutoFocus")));
  }
  btnminus.setText("－");
  btnplus.setText("＋");
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Git


GitHash::GitHash() : AbstractControl("커밋(로컬/리모트)", "", "") {

  QString lhash = QString::fromStdString(Params().get("GitCommit").substr(0, 10));
  QString rhash = QString::fromStdString(Params().get("GitCommitRemote").substr(0, 10));
  hlayout->addStretch(1);
  
  local_hash.setText(QString::fromStdString(Params().get("GitCommit").substr(0, 10)));
  remote_hash.setText(QString::fromStdString(Params().get("GitCommitRemote").substr(0, 10)));
  //local_hash.setAlignment(Qt::AlignVCenter);
  remote_hash.setAlignment(Qt::AlignVCenter);
  local_hash.setStyleSheet("color: #aaaaaa");
  if (lhash == rhash) {
    remote_hash.setStyleSheet("color: #aaaaaa");
  } else {
    remote_hash.setStyleSheet("color: #0099ff");
  }
  hlayout->addWidget(&local_hash);
  hlayout->addWidget(&remote_hash);
}

////////////////////////////////////////////////////////////////////////////////////////
//
//  QComboBox
CarSelectCombo::CarSelectCombo() : AbstractControl("Car", "자동차 모델을 강제로 인식시키는 메뉴입니다.", "") 
{
  combobox.setStyleSheet(R"(
    font-size: 35px;
    subcontrol-origin: padding;
    subcontrol-position: top right;
    selection-background-color: #111;
    selection-color: yellow;
    color: white;
    background-color: #393939;
    border-style: solid;
    border: 1px solid #1e1e1e;
    border-radius: 5;
    padding: 1px 0px 1px 5px; 
  )");

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");

  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");

   combobox.addItem("HYUNDAI ELANTRA LIMITED 2017");
    combobox.addItem("HYUNDAI I30 N LINE 2019");
    combobox.addItem("HYUNDAI GENESIS 2015-2016");

    combobox.addItem("HYUNDAI IONIQ ELECTRIC 2019");
    combobox.addItem("HYUNDAI IONIQ ELECTRIC 2020");
    combobox.addItem("HYUNDAI KONA 2020");
    combobox.addItem("HYUNDAI KONA ELECTRIC 2019");
    combobox.addItem("HYUNDAI SANTA FE LIMITED 2019");
    combobox.addItem("HYUNDAI SONATA 2020");
    combobox.addItem("HYUNDAI SONATA 2019");
    combobox.addItem("HYUNDAI PALISADE 2020");
    combobox.addItem("HYUNDAI VELOSTER 2019");
    combobox.addItem("HYUNDAI GRANDEUR HYBRID 2019");


    combobox.addItem("KIA FORTE E 2018 & GT 2021");
    combobox.addItem("KIA NIRO EV 2020");
    combobox.addItem("KIA OPTIMA SX 2019 & 2016");
    combobox.addItem("KIA OPTIMA HYBRID 2017 2019");
    combobox.addItem("KIA SELTOS 2021");
    combobox.addItem("KIA SORENTO GT LINE 2018");
    combobox.addItem("KIA STINGER GT2 2018");
    combobox.addItem("KIA CEED INTRO ED 2019");


    combobox.addItem("GENESIS G70 2018");
    combobox.addItem("GENESIS G80 2017");
    combobox.addItem("GENESIS G90 2017");

  //QAbstractItemView *qv = combobox.view();
  //QScrollBar *scrollbar = qv->verticalScrollBar();    

  hlayout->addWidget(&combobox);

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);


  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);



  QObject::connect(&btnminus, &QPushButton::released, [=]() 
  {
    int nIdx = combobox.currentIndex() - 1;
    if( nIdx < 0 ) nIdx = 0;
    combobox.setCurrentIndex( nIdx);
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::released, [=]() 
  {
    int nMax = combobox.count();
    int nIdx = combobox.currentIndex() + 1;

    if( nIdx >=  nMax )
      nIdx = nMax;

    combobox.setCurrentIndex( nIdx);

    refresh();
  });

  QObject::connect(&combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index)
  {
      int nIdx = combobox.currentIndex();

      QString str = combobox.currentText();
      printf("changeEvent: %d  index = %d %s \n", nIdx, index, str.toStdString().c_str() );
      refresh();
  });

  refresh();
}

void CarSelectCombo::refresh() 
{
   int nIdx = combobox.currentIndex();
  label.setText( QString::number(nIdx) );

  btnminus.setText("－");
  btnplus.setText("＋");
}