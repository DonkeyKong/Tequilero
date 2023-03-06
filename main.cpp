#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setStyle("fusion");
	MainWindow w;

#ifdef SIMULATE_PI_HARDWARE
	w.show();
#else
	w.showFullScreen();
#endif

	return a.exec();
}
