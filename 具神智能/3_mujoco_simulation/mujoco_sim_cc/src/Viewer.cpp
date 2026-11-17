#include "Viewer.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <algorithm>

Viewer::Viewer(Simulation* simulation,QWidget* parent)
    :QOpenGLWidget(parent),m_sim(simulation) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(900,600);
    mjv_defaultCamera(&m_camera);
    mjv_defaultOption(&m_options);
    mjv_defaultScene(&m_scene);
    mjr_defaultContext(&m_context);
    m_camera.type=mjCAMERA_FREE;
    m_camera.distance=1.25;
    m_camera.azimuth=135;
    m_camera.elevation=-25;
    m_camera.lookat[0]=.12;m_camera.lookat[1]=0;m_camera.lookat[2]=.12;
    connect(&m_timer,&QTimer::timeout,this,[this]{update();});
    m_timer.start(16);
    m_elapsed.start();
}
Viewer::~Viewer() {
    m_timer.stop();
    makeCurrent();
    if(m_ready){mjr_freeContext(&m_context);mjv_freeScene(&m_scene);}
    doneCurrent();
}
void Viewer::initializeGL() {
    // MuJoCo's renderer uses the OpenGL context created and owned by Qt.
    mjv_makeScene(m_sim->model(),&m_scene,2000);
    mjr_makeContext(m_sim->model(),&m_context,mjFONTSCALE_150);
    m_ready=true;
    connect(context(),&QOpenGLContext::aboutToBeDestroyed,this,[this]{
        makeCurrent();
        if(m_ready){mjr_freeContext(&m_context);mjv_freeScene(&m_scene);m_ready=false;}
        doneCurrent();
    });
}
void Viewer::resizeGL(int,int) {}
void Viewer::paintGL() {
    if(!m_ready)return;
    const double dt=std::clamp(m_elapsed.restart()/1000.,0.,.05);
    m_sim->update(dt);
    mjv_updateScene(m_sim->model(),m_sim->data(),&m_options,nullptr,&m_camera,mjCAT_ALL,&m_scene);
    const double dpr=devicePixelRatioF();
    const mjrRect rect{0,0,int(width()*dpr),int(height()*dpr)};
    // QOpenGLWidget renders to an internal FBO, not the platform default framebuffer.
    mjr_setBuffer(mjFB_WINDOW,&m_context);
    // Window mode selects standard viewport settings. Restore Qt widget FBO
    // after mjr_setBuffer(), since QOpenGLWidget does not draw to FBO 0.
    context()->extraFunctions()->glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());
    mjr_render(rect,&m_scene,&m_context);
}
void Viewer::mousePressEvent(QMouseEvent* e){m_last=e->position().toPoint();}
void Viewer::mouseMoveEvent(QMouseEvent* e){
    const QPoint now=e->position().toPoint();
    const double dx=double(now.x()-m_last.x())/std::max(1,width());
    const double dy=double(now.y()-m_last.y())/std::max(1,height());
    m_last=now;
    const auto buttons=e->buttons();
    if(!(buttons & (Qt::LeftButton|Qt::RightButton|Qt::MiddleButton)))return;
    const mjtMouse action=(buttons & Qt::RightButton)?mjMOUSE_MOVE_H:
                          (buttons & Qt::MiddleButton)?mjMOUSE_MOVE_V:
                          (e->modifiers() & Qt::ShiftModifier)?mjMOUSE_MOVE_H:mjMOUSE_ROTATE_V;
    mjv_moveCamera(m_sim->model(),action,dx,dy,&m_camera);
    update();
}
void Viewer::wheelEvent(QWheelEvent* e){
    const double dy=-.05*e->angleDelta().y()/120.;
    mjv_moveCamera(m_sim->model(),mjMOUSE_ZOOM,0,dy,&m_camera);
    update();
}
