#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include "QMainController.h"
#include "../ref/iris/src/iris_common.inl"

int main(int argc, char* argv[]) {
	QGuiApplication app(argc, argv);
	app.setOrganizationName(QStringLiteral("PaintDream"));
	app.setApplicationName(QStringLiteral("LeavesWing"));

	// Use Material style for a modern look across all platforms
	QQuickStyle::setStyle(QStringLiteral("Material"));

	QQmlApplicationEngine engine;

	// Expose main controller to QML
	QMainController* mainController = new QMainController(&app);
	engine.rootContext()->setContextProperty(QStringLiteral("mainController"), mainController);

	mainController->getAsyncWorker().queue([]() {
		qInfo() << "AsyncWorker message dispatched!";
	}, ~(size_t)0);

	QObject::connect(
		&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	engine.loadFromModule("LeavesWingQML", "Main");
#else
	engine.load(QUrl(QStringLiteral("qrc:/LeavesWingQML/src/qml/Main.qml")));
#endif

	return app.exec();
}
