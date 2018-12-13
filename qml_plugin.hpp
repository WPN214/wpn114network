#ifndef QML_PLUGIN_HPP
#define QML_PLUGIN_HPP

#include <QQmlExtensionPlugin>

class qml_plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA ( IID QQmlExtensionInterface_iid )

public:
    void registerTypes(const char* uri) override;
};

#endif // QML_PLUGIN_HPP
