#include <QElapsedTimer>
#include "TimingUtils.hpp"

static QElapsedTimer timer;

static struct _StartTimer {
    _StartTimer() {
        timer.start();
    };
} _startIt;

decltype(qDebug()) TimingUtils::log() {
    return qDebug() << timer.nsecsElapsed();
}
long TimingUtils::getNsecs() {
    return (long)timer.nsecsElapsed();
}
