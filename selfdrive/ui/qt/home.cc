#include "selfdrive/ui/qt/home.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QVBoxLayout>

#include <QProcess> // opkr

#include "selfdrive/common/params.h"
#include "selfdrive/common/swaglog.h"
#include "selfdrive/common/timing.h"
#include "selfdrive/common/util.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/widgets/drive_stats.h"
#include "selfdrive/ui/qt/widgets/setup.h"

// HomeWindow: the container for the offroad and onroad UIs

HomeWindow::HomeWindow(QWidget* parent) : QWidget(parent) {
  QHBoxLayout *main_layout = new QHBoxLayout(this);
  main_layout->setMargin(0);
  main_layout->setSpacing(0);

  sidebar = new Sidebar(this);
  main_layout->addWidget(sidebar);
  QObject::connect(this, &HomeWindow::update, sidebar, &Sidebar::updateState);
  QObject::connect(sidebar, &Sidebar::openSettings, this, &HomeWindow::openSettings);

  slayout = new QStackedLayout();
  main_layout->addLayout(slayout);

  onroad = new OnroadWindow(this);
  slayout->addWidget(onroad);

  QObject::connect(this, &HomeWindow::update, onroad, &OnroadWindow::update);
  QObject::connect(this, &HomeWindow::offroadTransitionSignal, onroad, &OnroadWindow::offroadTransitionSignal);

  home = new OffroadHome();
  slayout->addWidget(home);

  driver_view = new DriverViewWindow(this);
  connect(driver_view, &DriverViewWindow::done, [=] {
    showDriverView(false);
  });
  slayout->addWidget(driver_view);
}

void HomeWindow::showSidebar(bool show) {
  sidebar->setVisible(show);
}

void HomeWindow::offroadTransition(bool offroad) {
  if (offroad) {
    slayout->setCurrentWidget(home);
  } else {
    slayout->setCurrentWidget(onroad);
  }
  sidebar->setVisible(offroad);
  emit offroadTransitionSignal(offroad);
}

void HomeWindow::showDriverView(bool show) {
  if (show) {
    emit closeSettings();
    slayout->setCurrentWidget(driver_view);
  } else {
    slayout->setCurrentWidget(home);
  }
  sidebar->setVisible(show == false);
}

void HomeWindow::mousePressEvent(QMouseEvent* e) {
  // Handle sidebar collapsing
  // atom  mouse
  int e_x = e->x();
  int e_y = e->y();
  int e_button= e->button();
  // 1400, 820
  if( e_x < 500 || e_y < 300 ) 
  {
    // Handle sidebar collapsing
    bool bSidebar = sidebar->isVisible();
    if (onroad->isVisible() && (!bSidebar || e_x > sidebar->width())) {
    // Hide map first if visible, then hide sidebar
    if (onroad->map != nullptr && onroad->map->isVisible()){
      onroad->map->setVisible(false);
    } else if (!sidebar->isVisible()) {
      bSidebar = true;
    } else {
      bSidebar = false;

      if (onroad->map != nullptr) onroad->map->setVisible(true);
    }
    QUIState::ui_state.scene.mouse.sidebar = bSidebar;
    sidebar->setVisible(bSidebar);
    }
  }

   // OPKR Code
  if (QUIState::ui_state.scene.started && btn_map_overlay.ptInRect(e->x(), e->y())) {
    QProcess::execute("am start --activity-task-on-home com.opkr.maphack/com.opkr.maphack.MainActivity");
    QUIState::ui_state.scene.scr.map_on_overlay = true;

    QSoundEffect effect1;
    effect1.setSource(QUrl::fromLocalFile("/data/openpilot/selfdrive/assets/sounds/warning_1.wav"));
    effect1.play();
    return;
  }


  if ( QUIState::ui_state.scene.started && btn_Tmap.ptInRect(e->x(), e->y())) {

    if ( !QUIState::ui_state.scene.scr.map_is_running ) {
      QProcess::execute("am start com.skt.tmap.ku/com.skt.tmap.activity.TmapNaviActivity");
      QUIState::ui_state.scene.scr.map_is_running = true;
      QUIState::ui_state.scene.scr.map_on_overlay = false;
      Params().put("OpkrMapEnable", "1");
    } else {
      QProcess::execute("pkill com.skt.tmap.ku");
      QUIState::ui_state.scene.scr.map_is_running = false;
      Params().put("OpkrMapEnable", "0"); 
    }

    QSoundEffect effect2;
    effect2.setSource(QUrl::fromLocalFile("/data/openpilot/selfdrive/assets/sounds/warning_1.wav"));
    effect2.play();  
    return;
  }



  if (QUIState::ui_state.scene.mouse.sidebar )
  {
    e_x -= QUIState::ui_state.viz_rect.x + (bdr_s * 2) + 170;
  }
  QUIState::ui_state.scene.mouse.touch_x = e_x;
  QUIState::ui_state.scene.mouse.touch_y = e_y;
  QUIState::ui_state.scene.mouse.touched = e_button;
  QUIState::ui_state.scene.mouse.touch_cnt++;
  printf("mousePressEvent %d,%d  %d \n", e_x, e_y, e_button);
}

// OffroadHome: the offroad home page

OffroadHome::OffroadHome(QWidget* parent) : QFrame(parent) {
  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setMargin(50);

  // top header
  QHBoxLayout* header_layout = new QHBoxLayout();

  date = new QLabel();
  header_layout->addWidget(date, 0, Qt::AlignHCenter | Qt::AlignLeft);

  alert_notification = new QPushButton();
  alert_notification->setObjectName("alert_notification");
  alert_notification->setVisible(false);
  QObject::connect(alert_notification, &QPushButton::released, this, &OffroadHome::openAlerts);
  header_layout->addWidget(alert_notification, 0, Qt::AlignHCenter | Qt::AlignRight);

  QLabel* version = new QLabel(getBrandVersion());
  header_layout->addWidget(version, 0, Qt::AlignHCenter | Qt::AlignRight);

  main_layout->addLayout(header_layout);

  // main content
  main_layout->addSpacing(25);
  center_layout = new QStackedLayout();

  QHBoxLayout* statsAndSetup = new QHBoxLayout();
  statsAndSetup->setMargin(0);

  DriveStats* drive = new DriveStats;
  drive->setFixedSize(800, 800);
  statsAndSetup->addWidget(drive);

  SetupWidget* setup = new SetupWidget;
  statsAndSetup->addWidget(setup);

  QWidget* statsAndSetupWidget = new QWidget();
  statsAndSetupWidget->setLayout(statsAndSetup);

  center_layout->addWidget(statsAndSetupWidget);

  alerts_widget = new OffroadAlert();
  QObject::connect(alerts_widget, &OffroadAlert::closeAlerts, this, &OffroadHome::closeAlerts);
  center_layout->addWidget(alerts_widget);
  center_layout->setAlignment(alerts_widget, Qt::AlignCenter);

  main_layout->addLayout(center_layout, 1);

  // set up refresh timer
  timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, &OffroadHome::refresh);
  timer->start(10 * 1000);

  setStyleSheet(R"(
    * {
     color: white;
    }
    OffroadHome {
      background-color: black;
    }
    #alert_notification {
      padding: 15px;
      padding-left: 30px;
      padding-right: 30px;
      border: 1px solid;
      border-radius: 5px;
      font-size: 40px;
      font-weight: 500;
    }
    OffroadHome>QLabel {
      font-size: 55px;
    }
  )");
}

void OffroadHome::showEvent(QShowEvent *event) {
  refresh();
}

void OffroadHome::openAlerts() {
  center_layout->setCurrentIndex(1);
}

void OffroadHome::closeAlerts() {
  center_layout->setCurrentIndex(0);
}

void OffroadHome::refresh() {
  bool first_refresh = !date->text().size();
  if (!isVisible() && !first_refresh) {
    return;
  }

  date->setText(QDateTime::currentDateTime().toString("dddd, MMMM d"));

  // update alerts

  alerts_widget->refresh();
  if (!alerts_widget->alertCount && !alerts_widget->updateAvailable) {
    closeAlerts();
    alert_notification->setVisible(false);
    return;
  }

  if (alerts_widget->updateAvailable) {
    alert_notification->setText("UPDATE");
  } else {
    int alerts = alerts_widget->alertCount;
    alert_notification->setText(QString::number(alerts) + " ALERT" + (alerts == 1 ? "" : "S"));
  }

  if (!alert_notification->isVisible() && !first_refresh) {
    openAlerts();
  }
  alert_notification->setVisible(true);
  // Red background for alerts, blue for update available
  alert_notification->setStyleSheet(alerts_widget->updateAvailable ? "background-color: #364DEF" : "background-color: #E22C2C");
}
