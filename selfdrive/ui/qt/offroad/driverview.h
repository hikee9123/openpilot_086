#pragma once

#include <memory>

#include <QStackedLayout>

#include "selfdrive/common/util.h"
#include "selfdrive/ui/qt/widgets/cameraview.h"

class DriverViewScene : public QWidget {
  Q_OBJECT

public:
  explicit DriverViewScene(QWidget *parent);

public slots:
  void frameUpdated();

protected:
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  Params params;
  SubMaster sm;
  QImage face;
  bool is_rhd = false;
  bool frame_updated = false;

private:
  void  ui_print( QPainter &p, int x, int y,  const char* fmt, ... );
  void  ui_draw_driver(cereal::DriverState::Reader driver_state);  
};

class DriverViewWindow : public QWidget {
  Q_OBJECT

public:
  explicit DriverViewWindow(QWidget *parent);

signals:
  void done();

protected:
  void mousePressEvent(QMouseEvent* e) override;

private:
  CameraViewWidget *cameraView;
  DriverViewScene *scene;
  QStackedLayout *layout;


};
