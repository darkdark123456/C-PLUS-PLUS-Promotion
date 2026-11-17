#pragma once
#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QTimer>
#include <QPoint>
#include <mujoco/mujoco.h>
#include "Simulation.h"

class Viewer final : public QOpenGLWidget {
public:
    explicit Viewer(Simulation* simulation, QWidget* parent=nullptr);
    ~Viewer() override;
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width,int height) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
private:
    Simulation* m_sim;
    mjvCamera m_camera{};
    mjvOption m_options{};
    mjvScene m_scene{};
    mjrContext m_context{};
    QElapsedTimer m_elapsed;
    QTimer m_timer;
    QPoint m_last;
    bool m_ready=false;
};
