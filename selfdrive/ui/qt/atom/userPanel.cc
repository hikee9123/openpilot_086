
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


CUserPanel::CUserPanel(QWidget* parent) : QFrame(parent)
{
  QWidget *w = new QWidget(parent);  
  QVBoxLayout *main_layout = new QVBoxLayout(w);
  main_layout->setSpacing(30);

  // wifi + tethering buttons
  auto updateBtn = new ButtonControl("업데이트 체크 및 적용", "업데이트");
  QObject::connect(updateBtn, &ButtonControl::released, [=]() 
  { 
    const char* gitcommit = "/data/openpilot/gitcommit.sh";
    const char* gitpull = "/data/openpilot/gitpull.sh";

    std::system("chmod 777 /data/openpilot/gitcommit.sh");
    std::system("chmod 777 /data/openpilot/gitpull.sh");
    

    std::system( gitcommit );
    std::system("date '+%F %T' > /data/params/d/LastUpdateTime");
    QString desc = "";
    QString commit_local = QString::fromStdString(Params().get("GitCommit").substr(0, 7));
    QString commit_remote = QString::fromStdString(Params().get("GitCommitRemote").substr(0, 7));
 
    desc += QString("(로컬/리모트): %1/%2\n").arg(commit_local, commit_remote );
    if (commit_local == commit_remote) {
      desc += QString("로컬과 리모트가 일치합니다.");
    } else {
      desc += QString("업데이트가 있습니다.");
    }
    if (ConfirmationDialog::confirm(desc, this)) {
      Params().putBool("PutPrebuiltOn", 0);
      std::system( gitpull );
    }


  });
  main_layout->addWidget(updateBtn);




  auto exe_git_pull = new ButtonControl("Git Pull 실행", "실행");
  QObject::connect(exe_git_pull, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to git pull?", this)) 
          {
            std::system("git pull");
          }
  });
  main_layout->addWidget(exe_git_pull);

  auto exe_git_cancel = new ButtonControl("Git Pull 취소", "실행");
  QObject::connect(exe_git_cancel, &ButtonControl::released, [=]() 
  { 
        if (ConfirmationDialog::confirm("GitPull 이전 상태로 되돌립니다. 진행하시겠습니까?", this))
        {
          const char* gitpull_cancel = "/data/openpilot/gitpull_cancel.sh ''";
          std::system(gitpull_cancel);
        }
  });  
  main_layout->addWidget(exe_git_cancel);


 // main_layout->addWidget(horizontal_line());

  main_layout->setMargin(100);
  setLayout(main_layout);
  layout()->addWidget(horizontal_line());  
  layout()->addWidget(new GitHash());

  layout()->addWidget(horizontal_line());
  
  layout()->addWidget(new SshLegacyToggle());

  layout()->addWidget(horizontal_line());


  layout()->addWidget(new ParamControl("IsOpenpilotViewEnabled",
                                  "주행화면 미리보기",
                                  "오픈파일럿 주행화면을 미리보기 합니다.",
                                  "../assets/offroad/icon_eon.png",
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
          if (ConfirmationDialog::confirm("Are you sure you want to exec(test_car_interfaces.py)?", this)) 
          {
            std::system("python /data/openpilot/selfdrive/car/tests/test_car_interfaces.py");
          }
  });

  auto build_exe = new ButtonControl("build 실행", "실행",
                                        "/data/openpilot/selfdrive/manager/build.py 을 실행 합니다.");
  connect(build_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(build.py)?", this)) 
          {
            std::system("python /data/openpilot/selfdrive/manager/build.py");
          }
  });


  auto finger_exe = new ButtonControl("핑거 2.0 실행", "실행",
                                        "/data/openpilot/selfdrive/car/fw_versions.py 을 실행 합니다.");
  connect(finger_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(fw_versions.py)?", this)) 
          {
            std::system("python /data/openpilot/selfdrive/car/fw_versions.py");
          }
  });

  auto android_exe = new ButtonControl("Android Open", "Open",
                                        "Android를 Open 합니다.");
  connect(android_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(Android)?", this)) 
          {
            std::system("am start -a android.settings.SETTINGS");
          }
  });

  auto apk_exe = new ButtonControl("apk.py Install", "Install",
                                        "/data/openpilot/selfdrive/assets/addon/apk/apk.py 을 실행 합니다.");
  connect(apk_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(apk.py)?", this)) 
          {
            std::system("chmod 777 /data/openpilot/selfdrive/assets/addon/apk/apk.py");
            std::system("chmod 777 /data/openpilot/selfdrive/assets/addon/apk/gitcommit.sh");
            std::system("chmod 777 /data/openpilot/panda_flashing.sh");
            std::system("chmod 777 /data/openpilot/gitpull.sh");
            std::system("chmod 777 /data/openpilot/gitpull_cancel.sh");
            std::system("chmod 777 /data/openpilot/run_mixplorer.sh");
            std::system("python /data/openpilot/selfdrive/assets/addon/apk/apk.py");
          }
  });

  auto mixplorer_exe = new ButtonControl("com.mixplorer Open", "Open",
                                   "run_mixplorer.sh 을 실행 합니다.");
  connect(mixplorer_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(com.mixplorer)?", this)) 
          {
              std::system("/data/openpilot/run_mixplorer.sh");
            
          }
  });

  auto tmapopen_exe = new ButtonControl("NAVI Open", "Open",
                                   "NAVI 을 실행 합니다.");
  connect(tmapopen_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(NAVI Open)?", this)) 
          {
            Params().put("OpkrMapEnable", "1");
          }
  });

  auto tmapopen2_exe = new ButtonControl("NAVI2 Open", "Open",
                                   "NAVI2 실행후 Overlay mode로 변경합니다.");
  connect(tmapopen2_exe, &ButtonControl::released, [=]() 
  { 
        if (ConfirmationDialog::confirm("Are you sure you want to exec(NAVI2 Open)?", this)) 
        {
          Params().put("OpkrMapEnable", "2");
        }
  });  

  auto tmapclose_exe = new ButtonControl("NAVI Close", "Close",
                                   "NAVI 을 Close 합니다.");
  connect(tmapclose_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(NAVI Close)?", this)) 
          {
            Params().put("OpkrMapEnable", "0");
          }
  });

  auto softkey_exe = new ButtonControl("Soft Key Open", "Open",
                                   "Soft Key 을 실행 합니다.");
  connect(softkey_exe, &ButtonControl::released, [=]() 
  { 
          if (ConfirmationDialog::confirm("Are you sure you want to exec(Soft Key)?", this)) 
          {
            //std::system("pm enable com.gmd.hidesoftkeys")
            std::system("am start com.gmd.hidesoftkeys/com.gmd.hidesoftkeys.MainActivity");
          }
  });  

  for (auto btn : {car_interfaces, build_exe, finger_exe, android_exe, apk_exe, mixplorer_exe, tmapopen_exe, tmapopen2_exe, tmapclose_exe, softkey_exe}) {
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
  hlayout->addStretch(2);
  
  local_hash.setText(QString::fromStdString(Params().get("GitCommit").substr(0, 10)));
  remote_hash.setText(QString::fromStdString(Params().get("GitCommitRemote").substr(0, 10)));
  local_hash.setAlignment(Qt::AlignVCenter);
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