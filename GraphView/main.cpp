#include <qapplication.h>

#include "GraphicView.h"


int main(int argc,char** args) {
	QApplication app(argc, args);

	QGraphicsView* graphicView = new GraphicView();


	graphicView->show();
	return app.exec();
}