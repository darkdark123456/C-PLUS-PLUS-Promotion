#include "Replay.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <qmath.h>

namespace {
struct Matrix { QVector<std::array<double,7>> values; };
bool parseNpy(const QByteArray& b,Matrix* result,QString* error) {
    if(b.size()<12 || b.left(6)!=QByteArray("\x93NUMPY",6)) { *error="Not an NPY file";return false; }
    const auto* raw=reinterpret_cast<const uchar*>(b.constData());
    const int major=raw[6];
    const int hstart=major==1?10:12;
    if(major!=1 && major!=2 && major!=3) { *error="Unsupported NPY version";return false; }
    const quint32 headerLen=major==1?qFromLittleEndian<quint16>(raw+8):qFromLittleEndian<quint32>(raw+8);
    if(qint64(hstart)+headerLen>b.size()) { *error="NPY header truncated";return false; }
    const QString header=QString::fromLatin1(b.constData()+hstart,int(headerLen));
    const QRegularExpression dtypePattern(R"('descr'\s*:\s*'([^']+)')");
    const QRegularExpression shapePattern(R"('shape'\s*:\s*\(\s*(\d+)\s*,\s*(\d+)\s*\))");
    const auto dtype=dtypePattern.match(header), shape=shapePattern.match(header);
    if(!dtype.hasMatch() || !shape.hasMatch() || header.contains("'fortran_order': True")) {
        *error="Expected C-order 2D numpy array";return false;
    }
    const QString format=dtype.captured(1);
    const bool is64=(format=="<f8" || format=="=f8" || format=="|f8");
    const bool is32=(format=="<f4" || format=="=f4" || format=="|f4");
    if(!is64 && !is32) { *error="Only little-endian float32/float64 NPY supported";return false; }
    bool ok=false;
    const qint64 rows=shape.captured(1).toLongLong(&ok);
    if(!ok || rows<2 || rows>10000000 || shape.captured(2).toInt()!=7) {
        *error="traj must have shape (N>=2, 7)";return false;
    }
    const qint64 bytesPer=is64?8:4, offset=qint64(hstart)+headerLen;
    if(rows>(b.size()-offset)/(7*bytesPer)) { *error="NPY data truncated";return false; }
    result->values.reserve(int(rows));
    const uchar* p=raw+offset;
    for(qint64 i=0;i<rows;++i) {
        std::array<double,7> v{};
        for(int j=0;j<7;++j) {
            if(is64) { quint64 bits=qFromLittleEndian<quint64>(p);double x;std::memcpy(&x,&bits,8);v[size_t(j)]=x;p+=8; }
            else { quint32 bits=qFromLittleEndian<quint32>(p);float x;std::memcpy(&x,&bits,4);v[size_t(j)]=x;p+=4; }
        }
        result->values.append(v);
    }
    return true;
}
}
bool Replay::load(const QString& filename,QString* error) {
    // tar is Windows 10/11 built-in bsdtar. It unpacks .npz (a ZIP archive).
    // This avoids adding a new third-party ZIP library to the existing CMake dependencies.
    QTemporaryDir tmp;
    if(!tmp.isValid()) { *error="Cannot create temporary directory";return false; }
    QProcess proc;
    proc.start("tar",{"-xf",QFileInfo(filename).absoluteFilePath(),"-C",tmp.path()});
    if(!proc.waitForStarted(5000) || !proc.waitForFinished(30000) || proc.exitCode()!=0) {
        *error="Unable to unpack .npz with OS tar: "+QString::fromLocal8Bit(proc.readAllStandardError());return false;
    }
    QFile npy(QDir(tmp.path()).filePath("traj.npy"));
    if(!npy.open(QIODevice::ReadOnly)) { *error="NPZ does not contain traj.npy";return false; }
    Matrix mat;
    if(!parseNpy(npy.readAll(),&mat,error))return false;
    m_source.clear();m_source.reserve(mat.values.size());
    const double t0=mat.values.first()[0];
    for(const auto& v:mat.values) {
        Frame f;f.t=v[0]-t0;
        if(!std::isfinite(f.t) || (m_source.size() && f.t<m_source.last().t)) {
            *error="traj time column must be finite, ascending";return false;
        }
        for(int j=0;j<6;++j) f.raw[size_t(j)]=v[size_t(j+1)];
        m_source.append(f);
    }
    return true;
}
bool Replay::selectWindow(double start,double length,QString* error) {
    if(m_source.size()<2) { *error="Not enough trajectory frames";return false; }
    start=std::max(0.,start);
    const auto begin=std::lower_bound(m_source.cbegin(),m_source.cend(),start,
        [](const Frame& f,double value){return f.t<value;});
    if(begin==m_source.cend()) { *error="--start is past trajectory duration";return false; }
    int first=int(begin-m_source.cbegin());
    auto end=length>0?std::lower_bound(begin,m_source.cend(),start+length,
        [](const Frame& f,double value){return f.t<value;}):m_source.cend()-1;
    int last=int(end-m_source.cbegin());
    last=min(last,m_source.size()-1);
    if(last<=first) { *error="Selected replay interval contains fewer than two frames";return false; }
    m_frames=m_source.mid(first,last-first+1);
    const double base=m_frames.first().t;
    for(Frame& f:m_frames)f.t-=base;
    m_duration=m_frames.last().t;
    if(m_duration<=0) { *error="Replay duration must be positive";return false; }
    return true;
}
void Replay::setJointRanges(const std::array<std::array<double,2>,6>& ranges) {
    auto convert=[&](QVector<Frame>& frames) {
        for(Frame& f:frames)for(int j=0;j<6;++j) {
            const double lo=ranges[size_t(j)][0],hi=ranges[size_t(j)][1];
            const double u=f.raw[size_t(j)];
            f.rad[size_t(j)]=lo+(j==5?u/100.:(u+100.)/200.)*(hi-lo);
        }
    };
    convert(m_source);convert(m_frames);
    m_activityHint=0;
    for(int i=1;i<m_source.size();++i) {
        double mx=0;
        for(int j=0;j<6;++j)mx=std::max(mx,std::abs(m_source[i].rad[size_t(j)]-m_source[i-1].rad[size_t(j)]));
        if(mx>.004) {m_activityHint=m_source[i-1].t;break;}
    }
    if(m_frames.isEmpty())return;
    double minGrip=m_frames.first().raw[5],maxGrip=minGrip;
    for(const Frame& f:m_frames){minGrip=std::min(minGrip,f.raw[5]);maxGrip=std::max(maxGrip,f.raw[5]);}
    const double threshold=(maxGrip-minGrip)>8?minGrip+.30*(maxGrip-minGrip):maxGrip;
    for(Frame& f:m_frames)f.hold=f.raw[5]<=threshold;
    int intervals=0;
    for(int i=0;i<m_frames.size();) {
        if(!m_frames[i].hold){++i;continue;}
        int j=i;while(j<m_frames.size() && m_frames[j].hold)++j;
        if(j-i<5){for(int k=i;k<j;++k)m_frames[k].hold=false;}
        else ++intervals;
        i=j;
    }
    if(!intervals)for(Frame& f:m_frames)f.hold=true;
}
std::array<double,6> Replay::row(int index) const { return m_frames.at(index).rad; }
std::array<double,6> Replay::sample(double seconds) const {
    if(seconds<=0)return m_frames.first().rad;
    if(seconds>=m_duration)return m_frames.last().rad;
    auto it=std::lower_bound(m_frames.cbegin(),m_frames.cend(),seconds,
        [](const Frame& f,double value){return f.t<value;});
    const Frame& b=*it;const Frame& a=*(it-1);
    const double w=b.t>a.t?(seconds-a.t)/(b.t-a.t):0;
    std::array<double,6> out{};
    for(int j=0;j<6;++j)out[size_t(j)]=a.rad[size_t(j)]+w*(b.rad[size_t(j)]-a.rad[size_t(j)]);
    return out;
}
bool Replay::gripping(double seconds) const {
    auto it=std::upper_bound(m_frames.cbegin(),m_frames.cend(),seconds,
        [](double value,const Frame& f){return value<f.t;});
    return (it==m_frames.cbegin()?m_frames.cbegin():it-1)->hold;
}
int Replay::firstGripIndex() const {
    for(int i=0;i<m_frames.size();++i)if(m_frames[i].hold)return i;
    return 0;
}
