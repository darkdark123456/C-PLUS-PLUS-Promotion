#include "Simulation.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QQuaternion>
#include <QtMath>
#include <Eigen/Core>
#include <algorithm>
#include <cmath>

const std::array<const char*,6> Simulation::kJoints{
    "shoulder_pan", "shoulder_lift", "elbow_flex", "wrist_flex", "wrist_roll", "gripper"
};
namespace {
bool copyTree(const QString& src, const QString& dst) {
    const QDir from(src);
    if (!from.exists() || !QDir().mkpath(dst)) return false;
    for (const QFileInfo& fi : from.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString target = QDir(dst).filePath(fi.fileName());
        if (fi.isDir()) { if (!copyTree(fi.absoluteFilePath(), target)) return false; }
        else if (!QFile::copy(fi.absoluteFilePath(), target)) return false;
    }
    return true;
}
QMatrix4x4 matrixFromPose(const mjtNum* p, const mjtNum* R) {
    QMatrix4x4 result;
    for (int row=0;row<3;++row) {
        for (int col=0;col<3;++col) result(row,col)=float(R[3*row+col]);
        result(row,3)=float(p[row]);
    }
    return result;
}
QVector3D position(const QMatrix4x4& m) { return QVector3D(m(0,3),m(1,3),m(2,3)); }
double ease(double x) { return x*x*(3.-2.*x); }
}
Simulation::~Simulation() { if(m_data) mj_deleteData(m_data); if(m_model) mj_deleteModel(m_model); }

bool Simulation::loadModel(const QString& root, bool withSlider, QString* error) {
    QString file = QDir(root).filePath("scene.xml");
    if (!QFile::exists(file)) { *error="Missing scene.xml: "+file; return false; }
    if (withSlider) {
        // Keep the user's scene.xml and STL files untouched. Stage temporary copy.
        m_temp=std::make_unique<QTemporaryDir>();
        if (!m_temp->isValid() ||
            !QFile::copy(QDir(root).filePath("scene.xml"),QDir(m_temp->path()).filePath("scene.xml")) ||
            !QFile::copy(QDir(root).filePath("so101_new_calib.xml"),QDir(m_temp->path()).filePath("so101_new_calib.xml")) ||
            !copyTree(QDir(root).filePath("assets"),QDir(m_temp->path()).filePath("assets"))) {
            *error="Cannot stage scene assets into temporary directory"; return false;
        }
        file=QDir(m_temp->path()).filePath("scene.xml");
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { *error=f.errorString(); return false; }
        QString xml=QString::fromUtf8(f.readAll()); f.close();
        const QString slider=QStringLiteral(R"(
<body name="red_slider" pos="0.33 0 0">
  <freejoint/>
  <geom name="slider_geom" type="box" size="0.017 0.017 0.030"
        pos="0 0 0.030" rgba="1 0.08 0.08 1"
        friction="1.2 0.005 0.0001" mass="0.05"/>
</body>
)");
        if (!xml.contains("</worldbody>")) { *error="scene.xml has no closing worldbody"; return false; }
        xml.replace("</worldbody>",slider+"\n</worldbody>");
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { *error=f.errorString();return false; }
        f.write(xml.toUtf8()); f.close();
    }
    QByteArray bytes=QFile::encodeName(file);
    char message[1024]{};
    m_model=mj_loadXML(bytes.constData(),nullptr,message,sizeof(message));
    if (!m_model) { *error=QString::fromUtf8(message); return false; }
    m_data=mj_makeData(m_model);
    if (!m_data) { *error="mj_makeData failed";return false; }
    mj_resetData(m_model,m_data);
    mj_forward(m_model,m_data);
    if (withSlider) {
        const int id=mj_name2id(m_model,mjOBJ_JOINT,"red_slider");
        if (id<0) { *error="red_slider freejoint not found";return false; }
        m_sliderAdr=m_model->jnt_qposadr[id];
    }
    return true;
}
int Simulation::jointId(int index) const { return mj_name2id(m_model,mjOBJ_JOINT,kJoints[size_t(index)]); }
void Simulation::setArm(const Angles& values) {
    for(int c=0;c<6;++c) {
        const int id=jointId(c);
        if (id>=0) m_data->qpos[m_model->jnt_qposadr[id]]=values[size_t(c)];
    }
    mj_forward(m_model,m_data);
}
QMatrix4x4 Simulation::bodyPose(const char* name) const {
    const int id=mj_name2id(m_model,mjOBJ_BODY,name);
    if(id<0) return QMatrix4x4();
    return matrixFromPose(m_data->xpos+3*id,m_data->xmat+9*id);
}
QMatrix4x4 Simulation::sliderPose() const {
    QMatrix4x4 T;
    if(m_sliderAdr<0) return T;
    const mjtNum* q=m_data->qpos+m_sliderAdr;
    mjtNum R[9]; mju_quat2Mat(R,q+3);
    return matrixFromPose(q,R);
}
void Simulation::placeSlider(const QVector3D& at) {
    if(m_sliderAdr<0) return;
    mjtNum* q=m_data->qpos+m_sliderAdr;
    q[0]=at.x();q[1]=at.y();q[2]=at.z();
    q[3]=1.;q[4]=q[5]=q[6]=0.;
    mj_forward(m_model,m_data);
}
void Simulation::followGripper() {
    QMatrix4x4 T=bodyPose("gripper")*m_holdTransform;
    mjtNum* q=m_data->qpos+m_sliderAdr;
    for(int i=0;i<3;++i) q[i]=T(i,3);
    mjtNum R[9],quat[4];
    for(int i=0;i<3;++i) for(int j=0;j<3;++j) R[3*i+j]=T(i,j);
    mju_mat2Quat(quat,R);
    for(int i=0;i<4;++i) q[3+i]=quat[i];
    mj_forward(m_model,m_data);
}
double Simulation::sliderLowest() const {
    const mjtNum* q=m_data->qpos+m_sliderAdr;
    mjtNum R[9]; mju_quat2Mat(R,q+3);
    const double hx=kHalf[0],hy=kHalf[1],hz=kHalf[2];
    // Min of 8 corners: z + min sign contributions; bottom faces at z=0/2hz.
    return q[2]-std::abs(R[6])*hx-std::abs(R[7])*hy+std::min(0.,2*hz*R[8]);
}
QVector3D Simulation::fingerCenter() const {
    const std::array<const char*,2> meshes{"moving_jaw_so101_v1","wrist_roll_follower_so101_v1"};
    Eigen::Vector3d total=Eigen::Vector3d::Zero();
    double count=0;
    for (int g=0;g<m_model->ngeom;++g) {
        if(m_model->geom_type[g]!=mjGEOM_MESH) continue;
        const int id=m_model->geom_dataid[g];
        const char* name=mj_id2name(m_model,mjOBJ_MESH,id);
        if(!name || (qstrcmp(name,meshes[0])!=0 && qstrcmp(name,meshes[1])!=0)) continue;
        const int first=m_model->mesh_vertadr[id], n=m_model->mesh_vertnum[id];
        if(n<=0) continue;
        // Eigen evaluates the N x 3 mean; no per-vertex C++ transform loop.
        using VertexMatrix=Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor>;
        Eigen::Map<const VertexMatrix> vertices(m_model->mesh_vert+3*first,n,3);
        const Eigen::Vector3d center=vertices.colwise().mean().cast<double>();
        const mjtNum* r=m_data->geom_xmat+9*g;
        Eigen::Matrix3d rot;
        for(int i=0;i<3;++i) for(int j=0;j<3;++j) rot(i,j)=r[3*i+j];
        const mjtNum* p=m_data->geom_xpos+3*g;
        total+=double(n)*(rot*center+Eigen::Vector3d(p[0],p[1],p[2]));
        count+=n;
    }
    if(count<=0) return {};
    const Eigen::Vector3d c=total/count;
    return QVector3D(float(c.x()),float(c.y()),float(c.z()));
}
void Simulation::releaseSlider() {
    m_holding=false;
    m_sliderSpot=1-m_sliderSpot;
    placeSlider(m_spots[size_t(m_sliderSpot)]);
}
bool Simulation::buildScript(QString* error) {
    for(int i=0;i<6;++i) if(jointId(i)<0) { *error="Missing robot joint: "+QString::fromUtf8(kJoints[size_t(i)]);return false; }
    if(mj_name2id(m_model,mjOBJ_BODY,"gripper")<0) { *error="Missing body: gripper";return false; }
    const Angles home{0.,-.30,-.55,.35,0.,kOpen};
    const Angles graspA{0.,1.36,-1.40,1.16,0.,kOpen};
    Angles graspB=graspA; graspB[0]=kPan;
    Angles hover=graspA; hover[1]-=.55;
    Angles hoverB=hover;hoverB[0]=kPan;
    auto spot=[this](Angles q) { q[5]=kClosed;setArm(q);const QVector3D p=fingerCenter();return QVector3D(p.x(),p.y(),0); };
    m_spots={spot(graspA),spot(graspB)};
    m_phases={
        {"desc_A",graspA,kOpen,2.6,Phase::None},
        {"close_A",graspA,kClosed,.9,Phase::Grab},
        {"lift_A",hover,kClosed,2.2,Phase::None},
        {"pan_AB",hoverB,kClosed,2.8,Phase::None},
        {"down_B",graspB,kClosed,2.2,Phase::Place},
        {"open_B",graspB,kOpen,.9,Phase::None},
        {"up_B",hoverB,kOpen,2.,Phase::None},
        {"desc_B",graspB,kOpen,2.2,Phase::None},
        {"close_B",graspB,kClosed,.9,Phase::Grab},
        {"lift_B",hoverB,kClosed,2.,Phase::None},
        {"pan_BA",hover,kClosed,2.8,Phase::None},
        {"down_A",graspA,kClosed,2.2,Phase::Place},
        {"open_A",graspA,kOpen,.9,Phase::None},
        {"up_A",hover,kOpen,2.,Phase::None}
    };
    m_current=home;m_phaseStart=home;
    setArm(home);placeSlider(m_spots[0]);
    return true;
}
bool Simulation::buildReplay(QString* error) {
    for(int i=0;i<6;++i) if(jointId(i)<0) { *error="Missing robot joint: "+QString::fromUtf8(kJoints[size_t(i)]);return false; }
    std::array<std::array<double,2>,6> ranges{};
    for(int i=0;i<6;++i){const int id=jointId(i);ranges[size_t(i)]={m_model->jnt_range[2*id],m_model->jnt_range[2*id+1]};}
    m_replay.setJointRanges(ranges);
    setArm(m_replay.row(0));
    return true;
}
bool Simulation::initialize(const QString& root,const QString& replayFile,double start,double length,double speed,QString* error) {
    m_mode=replayFile.isEmpty()?Mode::Scripted:Mode::Replay;
    m_speed=speed>0?speed:1.;
    if(m_mode==Mode::Replay) {
        if(!m_replay.load(replayFile,error) || !m_replay.selectWindow(start,length,error))return false;
    }
    if(!loadModel(root,m_mode==Mode::Scripted,error))return false;
    if(m_mode==Mode::Scripted) return buildScript(error);
    return buildReplay(error);
}
void Simulation::advanceScript(double dt) {
    const Phase& phase=m_phases[m_phase];
    m_phaseClock=std::min(phase.duration,m_phaseClock+dt);
    const double x=ease(m_phaseClock/phase.duration);
    Angles q{};
    for(int i=0;i<6;++i)q[size_t(i)]=m_phaseStart[size_t(i)]+(phase.target[size_t(i)]-m_phaseStart[size_t(i)])*x;
    q[5]=m_phaseStart[5]+(phase.grip-m_phaseStart[5])*x;
    setArm(q);m_current=q;
    if(m_holding) {
        followGripper();
        if(phase.event==Phase::Place && sliderLowest()<=.0012) {
            releaseSlider();m_phase=(m_phase+1)%m_phases.size();m_phaseClock=0;m_phaseStart=m_current;return;
        }
    } else placeSlider(m_spots[size_t(m_sliderSpot)]);
    if(m_phaseClock>=phase.duration) {
        if(phase.event==Phase::Grab) {
            // Transform of the box relative to gripper, at the instant of grasp.
            m_holdTransform=bodyPose("gripper").inverted()*sliderPose();
            m_holding=true;
        } else if(phase.event==Phase::Place) releaseSlider();
        m_phase=(m_phase+1)%m_phases.size();
        m_phaseStart=m_current;m_phaseClock=0;
    }
}
void Simulation::advanceReplay(double dt) {
    m_clock+=dt*m_speed;
    if(m_clock>=m_replay.duration())m_clock=std::fmod(m_clock,m_replay.duration());
    setArm(m_replay.sample(m_clock));
}
void Simulation::update(double dt) {
    if(!m_model || !m_data)return;
    dt=std::clamp(dt,0.,.05);
    if(m_mode==Mode::Scripted)advanceScript(dt);
    else advanceReplay(dt);
}
void Simulation::reset() {
    if(!m_model)return;
    m_clock=0;m_phaseClock=0;m_phase=0;m_sliderSpot=0;m_holding=false;
    if(m_mode==Mode::Scripted) {m_phaseStart={0.,-.30,-.55,.35,0.,kOpen};m_current=m_phaseStart;setArm(m_current);placeSlider(m_spots[0]);}
    else setArm(m_replay.row(0));
}
QString Simulation::stage() const {
    if(m_mode==Mode::Replay)return QStringLiteral("Replay %1 / %2 s").arg(m_clock,0,'f',1).arg(m_replay.duration(),0,'f',1);
    return m_phases.isEmpty()?QStringLiteral("Idle"):QString::fromUtf8(m_phases[m_phase].label);
}
