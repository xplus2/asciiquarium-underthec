#include "plasma_item.h"

#include <array>

#include <QByteArray>
#include <QDateTime>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QPainter>
#include <QtMath>

extern "C" {
#include "config.h"
#include "scene.h"
}

namespace {

const int kMinFontPx = 7;

/* [enum color * 2 + bold] as 0xRRGGBB */
const std::array<QRgb, 18> kPalette = {
    0xc0c0c0, 0xffffff,
    0x000000, 0x808080,
    0xc00000, 0xff5555,
    0x00c000, 0x55ff55,
    0xc0c000, 0xffff55,
    0x0000c0, 0x5555ff,
    0xc000c0, 0xff55ff,
    0x00c0c0, 0x55ffff,
    0xc0c0c0, 0xffffff,
};

QColor colorOf(enum color col, bool bold) {
  return QColor(kPalette[static_cast<size_t>(col) * 2 + (bold ? 1 : 0)]);
}

double nowSeconds() {
  return static_cast<double>(QDateTime::currentMSecsSinceEpoch()) / 1000.0;
}

void applyOption(struct config *cfg, const QString &name, const QString &value) {
  char err[256];
  if (value.isEmpty()) return;
  if (!config_set(cfg, name.toUtf8().constData(), value.toUtf8().constData(), name.toUtf8().constData(), err, sizeof err)) qWarning("underthec: %s", err);
}

}

UnderTheC::UnderTheC(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setOpaquePainting(true);
  timer_.setTimerType(Qt::PreciseTimer);
  connect(&timer_, &QTimer::timeout, this, &UnderTheC::step);
}

UnderTheC::~UnderTheC() {
  stop();
}

void UnderTheC::setOptions(const QVariantMap &options) {
  if (options_ == options) return;
  options_ = options;
  emit optionsChanged();
  if (complete_) restart();
}

void UnderTheC::setFontSize(int fontSize) {
  if (fontSize_ == fontSize) return;
  fontSize_ = fontSize;
  emit fontSizeChanged();
  layout();
  update();
}

void UnderTheC::componentComplete() {
  QQuickPaintedItem::componentComplete();
  complete_ = true;
  restart();
}

void UnderTheC::stop() {
  timer_.stop();
  if (!started_) return;
  app_free(&app_);
  started_ = false;
}

void UnderTheC::restart() {
  stop();
  struct config cfg;
  config_init(&cfg);
  for (auto it = options_.cbegin(); it != options_.cend(); ++it) applyOption(&cfg, it.key(), it.value().toString());
  char err[256];
  if (!config_check(&cfg, err, sizeof err)) {
    qWarning("underthec: %s", err);
    config_free(&cfg);
    config_init(&cfg);
  }
  config_start(&cfg, &app_, nowSeconds());
  config_free(&cfg);
  started_ = true;
  timer_.setInterval(1000 / cfg.fps);
  layout();
  if (isVisible()) timer_.start();
  update();
}

void UnderTheC::layout() {
  if (!started_ || width() < 1 || height() < 1) return;
  font_ = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font_.setPixelSize(fontSize_ > 0 ? qMax(kMinFontPx, fontSize_) : qMax(10, static_cast<int>(height() / 45)));
  boldFont_ = font_;
  boldFont_.setBold(true);
  QFontMetricsF fm(font_);
  cellW_ = qMax(1, qCeil(fm.horizontalAdvance(QLatin1Char('M'))));
  cellH_ = qMax(1, qCeil(fm.height()));
  ascent_ = qRound(fm.ascent());
  int cols = qMax(1, static_cast<int>(width()) / cellW_);
  int rows = qMax(1, static_cast<int>(height()) / cellH_);
  offX_ = (static_cast<int>(width()) - cols * cellW_) / 2;
  offY_ = (static_cast<int>(height()) - rows * cellH_) / 2;
  app_resize(&app_, cols, rows);
}

void UnderTheC::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
  QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
  if (newGeometry.size() != oldGeometry.size()) layout();
}

void UnderTheC::itemChange(ItemChange change, const ItemChangeData &value) {
  QQuickPaintedItem::itemChange(change, value);
  if (change != ItemVisibleHasChanged || !started_) return;
  if (value.boolValue) timer_.start();
  else timer_.stop();
}

void UnderTheC::step() {
  app_frame(&app_, nowSeconds());
  update();
}

void UnderTheC::paint(QPainter *painter) {
  painter->fillRect(boundingRect(), Qt::black);
  if (!started_) return;
  const struct canvas &c = app_.canvas;
  for (int row = 0; row < c.height; row++) {
    for (int col = 0; col < c.width; col++) {
      const struct cell &cell = c.cells[static_cast<size_t>(row) * static_cast<size_t>(c.width) + static_cast<size_t>(col)];
      if (cell.bg == COL_DEFAULT) continue;
      painter->fillRect(offX_ + col * cellW_, offY_ + row * cellH_, cellW_, cellH_, colorOf(cell.bg, cell.bg_bold));
    }
  }
  painter->setRenderHint(QPainter::TextAntialiasing);
  for (int row = 0; row < c.height; row++) {
    for (int col = 0; col < c.width; col++) {
      const struct cell &cell = c.cells[static_cast<size_t>(row) * static_cast<size_t>(c.width) + static_cast<size_t>(col)];
      if (cell.cont || cell.glyph[0] == ' ' || cell.glyph[0] == '\0') continue;
      painter->setFont(cell.bold ? boldFont_ : font_);
      painter->setPen(colorOf(cell.col, cell.bold));
      painter->drawText(QPointF(offX_ + col * cellW_, offY_ + row * cellH_ + ascent_), QString::fromUtf8(cell.glyph));
    }
  }
}

QStringList UnderTheCInfo::creatures() const {
  QStringList names;
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) names << QString::fromLatin1(scene_aquatic_flag_name(i));
  return names;
}
