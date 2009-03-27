#include "tellduscenterapplication.h"
#include <QMessageBox>
#include <QDir>
#include <QIcon>
#include <QPluginLoader>
#include <QScriptEngine>

#include <QDebug>
#include <telldus-core.h>
#include "tellduscenterplugin.h"

class TelldusCenterPlugin;

typedef QList<TelldusCenterPlugin *> PluginList;


class TelldusCenterApplicationPrivate {
public:
	PluginList plugins;
	QPointer<MainWindow> mainWindow;
	QScriptEngine scriptEngine;
};

TelldusCenterApplication::TelldusCenterApplication(int &argc, char **argv)
		:QApplication(argc, argv)
{
	d = new TelldusCenterApplicationPrivate;

	//We use queued connection since it is called from another thread
	connect(this, SIGNAL(sigDeviceEvent(int, int, const QString &)), this, SLOT(deviceEvent(int, int, const QString &)), Qt::QueuedConnection);
	tdInit();
	tdRegisterDeviceEvent( &TelldusCenterApplication::deviceEvent, 0 );

}

TelldusCenterApplication::~TelldusCenterApplication() {
	qDeleteAll(d->plugins);
	delete d;
	tdClose();
}

PluginList TelldusCenterApplication::plugins() const {
	if (d->plugins.empty()) {
		return PluginList();
	}
	return d->plugins;
}

void TelldusCenterApplication::showMainWindow() {
	if (!isMainWindowShown()) {
		d->mainWindow = new MainWindow();
	}
	d->mainWindow->show();
}

bool TelldusCenterApplication::isMainWindowShown() {
	return !d->mainWindow.isNull();
}

#if defined(Q_WS_MAC)
bool TelldusCenterApplication::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::ApplicationActivate: {
		showMainWindow();
		return true;
	}
	default:
		break;
	}
	return QApplication::event(event);
}
#endif

void TelldusCenterApplication::eventTriggered( const QString &name, const QString &title ) {
	qDebug() << "Systray - eventTriggered:" << name << title;
}

void TelldusCenterApplication::deviceEvent(int deviceId, int method, const char *data, int /*callbackId*/, void */*context*/) {
	TelldusCenterApplication *app = TelldusCenterApplication::instance();
	emit app->sigDeviceEvent(deviceId, method, data);
}

void TelldusCenterApplication::loadPlugins() {
	QDir pluginsDir = QDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
	if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
		pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
	if (pluginsDir.dirName() == "MacOS") {
		pluginsDir.cdUp();
	}
#endif
	if (!pluginsDir.cd("Plugins")) {
		return;
	}
	this->setLibraryPaths( QStringList(pluginsDir.absolutePath()) );

	QScriptValue object = d->scriptEngine.newQObject(this);
	d->scriptEngine.globalObject().setProperty("application", object);

	foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
		QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
		QObject *plugin = loader.instance();
		if (plugin) { //Load the plugin
			loadPlugin(plugin);
		}
	}
}

void TelldusCenterApplication::loadPlugin(QObject *plugin) {
	TelldusCenterPlugin *iPlugin = qobject_cast<TelldusCenterPlugin *>(plugin);
	if (!iPlugin) {
		return;
	}
	QObject *qObjectPlugin = qobject_cast<QObject *>(plugin);
	if (qObjectPlugin) {
		if (qObjectPlugin->metaObject()->indexOfSignal( QMetaObject::normalizedSignature("hej()") ) >= 0) {
			connect(qObjectPlugin, SIGNAL(hej()), this, SLOT(aboutQt()));
		}
	}
	d->plugins.append(iPlugin);
}

void TelldusCenterApplication::loadScripts() {
	foreach (QString extension, d->scriptEngine.availableExtensions()) {
		d->scriptEngine.importExtension( extension );
		if (d->scriptEngine.hasUncaughtException()) {
			qDebug() << QString("Error in %1:%2:").arg(extension).arg(d->scriptEngine.uncaughtExceptionLineNumber())
					<< d->scriptEngine.uncaughtException().toString();
		}
	}
}

void TelldusCenterApplication::deviceEvent(int deviceId, int method, const QString &/*data*/) {
	char *name = tdGetName(deviceId);
	QString deviceName(name);
	free(name);
	QString methodName;
	switch( method ) {
	case TELLSTICK_TURNON:
		methodName = "turned on";
		break;
	case TELLSTICK_TURNOFF:
		methodName = "turned off";
		break;
	case TELLSTICK_BELL:
		methodName = "belled";
		break;
	case TELLSTICK_DIM:
		methodName = "dimmed to level x%";
		break;
	}
	showMessage("", QString("%1 %2").arg(deviceName).arg(methodName), "");
}

TelldusCenterApplication *TelldusCenterApplication::instance() {
	return (static_cast<TelldusCenterApplication *>(QCoreApplication::instance()));
}