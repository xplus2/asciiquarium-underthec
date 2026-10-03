#ifndef UNDERTHEC_PLASMA_ITEM_H
#define UNDERTHEC_PLASMA_ITEM_H

#include <QFont>
#include <QQuickPaintedItem>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

extern "C" {
#include "app.h"
}

class UnderTheC : public QQuickPaintedItem {
  Q_OBJECT
  /* long CLI option name -> value, empty values skipped */
  Q_PROPERTY(QVariantMap options READ options WRITE setOptions NOTIFY optionsChanged)
  /* px, 0=auto from item height, 1-6 act as 7 */
  Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)

public:
  explicit UnderTheC(QQuickItem *parent = nullptr);
  ~UnderTheC() override;

  void paint(QPainter *painter) override;

  QVariantMap options() const { return options_; }
  void setOptions(const QVariantMap &options);
  int fontSize() const { return fontSize_; }
  void setFontSize(int fontSize);

signals:
  void optionsChanged();
  void fontSizeChanged();

protected:
  void componentComplete() override;
  void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
  void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
  void restart();
  void stop();
  void layout();
  void step();

  struct app app_ = {};
  bool started_ = false;
  bool complete_ = false;
  QTimer timer_;
  QVariantMap options_;
  int fontSize_ = 0;
  QFont font_;
  QFont boldFont_;
  int cellW_ = 1;
  int cellH_ = 1;
  int ascent_ = 1;
  int offX_ = 0;
  int offY_ = 0;
};

/* creature names for the settings page */
class UnderTheCInfo : public QObject {
  Q_OBJECT
  Q_PROPERTY(QStringList creatures READ creatures CONSTANT)

public:
  explicit UnderTheCInfo(QObject *parent = nullptr) : QObject(parent) {}
  QStringList creatures() const;
};

#endif
