#ifndef WALL_TIME_QWT_SCALE_DRAW_H
#define WALL_TIME_QWT_SCALE_DRAW_H

//System includes
#ifdef _WIN32
#include <stdint.h>

#ifndef int64_t
typedef __int64 int64_t;
#endif

#else
#include <inttypes.h>
#endif

//Library includes
#include <qwt_scale_draw.h>

//Local includes
#include "../../AVNUtilLibs/Timestamp/Timestamp.h"

//A derived Qwt class to draw custom time labels on the QwtPlot X axis
class cWallTimeQwtScaleDraw : public QwtScaleDraw
{
public:
    cWallTimeQwtScaleDraw();

    virtual QwtText label(double dValue_s) const;
};

#endif // WALL_TIME_QWT_SCALE_DRAW_H
