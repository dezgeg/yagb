#include <QDebug>

class TimingUtils {
public:
    static decltype(qDebug()) log();
    static long getNsecs();
};
