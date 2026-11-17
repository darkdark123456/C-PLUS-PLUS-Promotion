#pragma once
#include <QString>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTemporaryDir>
#include <array>
#include <memory>
#include <mujoco/mujoco.h>
#include "Replay.h"

class Simulation final {
public:
    enum class Mode { Scripted, Replay };
    Simulation() = default;
    ~Simulation();
    bool initialize(const QString& resourceRoot, const QString& replayFile,
                    double start, double length, double speed, QString* error);
    void update(double dt);
    mjModel* model() const { return m_model; }
    mjData* data() const { return m_data; }
    QString stage() const;
    Mode mode() const { return m_mode; }
    void reset();
private:
    using Angles = std::array<double, 6>;
    struct Phase { const char* label; Angles target; double grip; double duration; enum Event { None, Grab, Place } event; };
    static constexpr double kOpen=1.7453, kClosed=.95, kPan=.9;
    static constexpr std::array<double,3> kHalf{.017,.017,.030};
    static const std::array<const char*,6> kJoints;
    mjModel* m_model=nullptr;
    mjData* m_data=nullptr;
    Mode m_mode=Mode::Scripted;
    Replay m_replay;
    double m_speed=1, m_clock=0, m_phaseClock=0;
    QVector<Phase> m_phases;
    Angles m_phaseStart{}, m_current{};
    int m_phase=0, m_sliderAdr=-1, m_sliderSpot=0;
    bool m_holding=false;
    QMatrix4x4 m_holdTransform;
    std::array<QVector3D,2> m_spots{};
    QVector3D m_lastSpot;
    std::unique_ptr<QTemporaryDir> m_temp;
    bool loadModel(const QString& root, bool withSlider, QString* error);
    bool buildScript(QString* error);
    bool buildReplay(QString* error);
    void setArm(const Angles& values);
    int jointId(int index) const;
    QVector3D fingerCenter() const;
    QMatrix4x4 bodyPose(const char* name) const;
    QMatrix4x4 sliderPose() const;
    void placeSlider(const QVector3D& at);
    void followGripper();
    double sliderLowest() const;
    void releaseSlider();
    void advanceScript(double dt);
    void advanceReplay(double dt);
    void resetReplay();
};
