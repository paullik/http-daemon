#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QSettings>

#include "iplugin.h"

#define CONFIG_FILE_NAME "httpdaemon.conf"

class Configuration
{
public:
    explicit Configuration(const QString &iniPath);
    ~Configuration();
    bool read();
    static QVariant get(const QString &key, QVariant defaultValue = QVariant());
    static QStringList getPluginKeys();
    static QPair<QString, IPlugin *> getPluginInfo(const QString &key);
    QString getSettingsPath() const;
    bool check() const;
protected:
    QString getPluginName(const QString &fullName) const;
    void loadPlugins(const QHash<QString, QString> &confPlugins);
    QString dereference(const QString &value, const QSettings &settings);
private:
    const QString settingsPath;
    static QHash<QString, QVariant> conf;
    static QHash<QString, QPair<QString, IPlugin*> > plugins;
};

#endif // CONFIGURATION_H
