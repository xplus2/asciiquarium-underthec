#include "plasma_item.h"

#include <QQmlExtensionPlugin>
#include <qqml.h>

class UnderTheCPlugin : public QQmlExtensionPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
  void registerTypes(const char *uri) override {
    qmlRegisterType<UnderTheC>(uri, 1, 0, "UnderTheC");
    qmlRegisterType<UnderTheCInfo>(uri, 1, 0, "UnderTheCInfo");
  }
};

#include "plasma_plugin.moc"
