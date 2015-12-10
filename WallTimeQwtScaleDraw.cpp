//System includes

//Library includes

//Local includes
#include "WallTimeQwtScaleDraw.h"

cWallTimeQwtScaleDraw::cWallTimeQwtScaleDraw()
{
}

QwtText cWallTimeQwtScaleDraw::label(double dValue_s) const
{
    //The supplied value is whole seconds elapsed today
    //Print a HH:mm:ss label

    return QwtText( QString(AVN::stringFromTimestamp_HHmmss((int64_t)(dValue_s * 1e6)).c_str()) );
}
