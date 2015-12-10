#ifndef QWT_PLOT_DISTANCE_PICKER_H
#define QWT_PLOT_DISTANCE_PICKER_H

//System includes

//Library includes
#include <QPainter>
#include <QString>

//Local includes
#include "QwtPlotPositionPicker.h"

class cQwtPlotDistancePicker : public cQwtPlotPositionPicker
{
    Q_OBJECT
public:
#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    explicit cQwtPlotDistancePicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QwtPlotCanvas* pCanvas);
#else
    explicit cQwtPlotDistancePicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget* pCanvas);
#endif

protected:
    virtual QwtText trackerTextF(const QPointF &oPosition) const;
};

#endif // BASIC_QWT_LINE_PLOT_DISTANCE_PICKER_H
