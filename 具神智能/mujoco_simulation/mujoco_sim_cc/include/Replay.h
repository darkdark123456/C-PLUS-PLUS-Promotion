#pragma once
#include <QVector>
#include <QString>
#include <array>

// NumPy .npz from sim_teleop.py: traj, shape (N,7), [time, 6 normalized joints].
class Replay final {
public:
    bool load(const QString& filename, QString* error);
    bool selectWindow(double start, double length, QString* error);
    std::array<double, 6> sample(double seconds) const;
    bool gripping(double seconds) const;
    std::array<double, 6> row(int index) const;
    int firstGripIndex() const;
    int length() const { return m_frames.size(); }
    double duration() const { return m_duration; }
    double activityHint() const { return m_activityHint; }
    void setJointRanges(const std::array<std::array<double, 2>, 6>& ranges);
private:
    struct Frame { double t = 0; std::array<double,6> raw{}; std::array<double,6> rad{}; bool hold = false; };
    QVector<Frame> m_source, m_frames;
    double m_duration = 0, m_activityHint = 0;
};
