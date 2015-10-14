//System includes
#include <iostream>
#include <cmath>

//Library includes
#include <qwt_picker_machine.h>

//Local includes
#include "BasicQwtLinePlotDistancePicker.h"

cBasicQwtLinePlotDistancePicker::cBasicQwtLinePlotDistancePicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget *pCanvas) :
    cBasicQwtLinePlotPositionPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
{
    setStateMachine( new QwtPickerDragRectMachine );
    setRubberBandPen(QPen(Qt::cyan));
    setMousePattern( QwtEventPattern::MouseSelect1, Qt::RightButton);
}

QwtText cBasicQwtLinePlotDistancePicker::trackerTextF(const QPointF &oPosition) const
{
    Q_UNUSED(oPosition);

    const QPolygon& oPolygon = selection();

    if ( oPolygon.size() != 2 )
    {
        return QwtText();
    }

    double dWidth = std::fabs( invTransform(oPolygon[1]).x() - invTransform( oPolygon[0]).x() );
    double dHeight = std::fabs( invTransform(oPolygon[1]).y() - invTransform(oPolygon[0]).y() );

    QwtText oTrackerText( QString("dX: %1 %2\ndY: %3 %4").arg(QString::number(dWidth)).arg(m_qstrXUnit).arg(QString::number(dHeight)).arg(m_qstrYUnit) );

    QColor oBackgroundColour(Qt::black);
    oBackgroundColour.setAlphaF(0.5);
    oTrackerText.setBackgroundBrush(QBrush(oBackgroundColour));
    oTrackerText.setColor(Qt::cyan);

    return oTrackerText;
}
