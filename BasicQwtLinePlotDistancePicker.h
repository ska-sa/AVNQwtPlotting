#ifndef BASIC_QWT_LINE_PLOT_DISTANCE_PICKER_H
#define BASIC_QWT_LINE_PLOT_DISTANCE_PICKER_H

//System includes

//Library includes
#include <QPainter>
#include <QString>

//Local includes
#include "BasicQwtLinePlotPositionPicker.h"

class cBasicQwtLinePlotDistancePicker : public cBasicQwtLinePlotPositionPicker
{
    Q_OBJECT
public:
    explicit cBasicQwtLinePlotDistancePicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget* pCanvas);

protected:
    virtual QwtText trackerTextF(const QPointF &oPosition) const;
};

#endif // BASIC_QWT_LINE_PLOT_DISTANCE_PICKER_H
