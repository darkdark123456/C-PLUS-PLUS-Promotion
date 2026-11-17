#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "Simulation.h"
#include "Viewer.h"

int main(int argc,char** argv) {
    // Qt OpenGL must be compatibility profile for MuJoCo's GL render path.
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(3,3);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc,argv);
    QCoreApplication::setApplicationName("SO-ARM101 Qt6");
    QCommandLineParser parser;
    parser.setApplicationDescription("MuJoCo SO-ARM101 Qt6 viewer");
    parser.addHelpOption();
    const QCommandLineOption resourceOpt("resource", "Directory containing scene.xml", "directory",SO_ARM101_RESOURCE_ROOT);
    const QCommandLineOption replayOpt("replay","Replay .npz trajectory (arm-only)","file");
    const QCommandLineOption speedOpt("speed","Replay speed","factor","1.0");
    const QCommandLineOption startOpt("start","Start offset in seconds","seconds","0");
    const QCommandLineOption lengthOpt("len","Replay duration in seconds","seconds","-1");
    parser.addOptions({resourceOpt,replayOpt,speedOpt,startOpt,lengthOpt});
    parser.process(app);

    Simulation sim;
    QString error;
    if(!sim.initialize(parser.value(resourceOpt),parser.value(replayOpt),
                      parser.value(startOpt).toDouble(),parser.value(lengthOpt).toDouble(),
                      parser.value(speedOpt).toDouble(),&error)) {
        QMessageBox::critical(nullptr,"Simulation initialization failed",error);
        return 1;
    }
    QMainWindow win;
    auto* central=new QWidget(&win);
    auto* vertical=new QVBoxLayout(central);
    auto* viewer=new Viewer(&sim,central);
    vertical->addWidget(viewer,1);
    auto* bar=new QHBoxLayout;
    auto* info=new QLabel(central);
    auto* reset=new QPushButton("Reset",central);
    bar->addWidget(info,1);bar->addWidget(reset);
    vertical->addLayout(bar);
    QObject::connect(reset,&QPushButton::clicked,&win,[&]{sim.reset();viewer->update();});
    QTimer status;
    QObject::connect(&status,&QTimer::timeout,&win,[&]{info->setText(sim.stage()+" | drag: rotate | right drag: pan | wheel: zoom");});
    status.start(100);
    win.setCentralWidget(central);
    win.resize(1100,780);
    win.setWindowTitle("SO-ARM101 | Qt6 + MuJoCo");
    win.show();
    return app.exec();
}
