//System includes
#include <iostream>
#include <cmath>

//Library includes
#include <qwt_picker_machine.h>

//Local includes
#include "QwtPlotDistancePicker.h"

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
cQwtPlotDistancePicker::cQwtPlotDistancePicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QwtPlotCanvas *pCanvas) :
    cQwtPlotPositionPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
  #else
cQwtPlotDistancePicker::cQwtPlotDistancePicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget *pCanvas) :
    cQwtPlotPositionPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
  #endif
{
    setStateMachine( new QwtPickerDragRectMachine );
    setRubberBandPen(QPen(Qt::cyan));
    setMousePattern( QwtEventPattern::MouseSelect1, Qt::RightButton);
}

QwtText cQwtPlotDistancePicker::trackerTextF(const QPointF &oPosition) const
{
    Q_UNUSED(oPosition);

    const QPolygon& oPolygon = selection();

    if ( oPolygon.size() != 2 )
    {
        return QwtText();
    }

    double dWidth = std::fabs( invTransform(oPolygon[1]).x() - invTransform( oPolygon[0]).x() );
    double dHeight = std::fabs( invTransform(oPolygon[1]).y() - invTransform(oPolygon[0]).y() );

    QString qstrX;
    QString qstrY;

    if(m_bXIsTime)
    {
        qstrX = QString("%1 %2").arg(QString::number(dWidth)).arg("s");
    }
    else
    {
        qstrX = QString("%1 %2").arg(QString::number(dWidth)).arg(m_qstrXUnit);
    }

    if(m_bYIsTime)
    {
        qstrY = QString("%1 %2").arg(QString::number(dHeight)).arg("s");
    }
    else
    {
        qstrY = QString("%1 %2").arg(QString::number(dHeight)).arg(m_qstrYUnit);
    }

    QwtText oTrackerText(QString("dX %1\ndY %2").arg(qstrX).arg(qstrY));

    QColor oBackgroundColour(Qt::black);
    oBackgroundColour.setAlphaF(0.5);
    oTrackerText.setBackgroundBrush(QBrush(oBackgroundColour));
    oTrackerText.setColor(Qt::cyan);

    return oTrackerText;
}
