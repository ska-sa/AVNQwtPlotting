//Overrides the regular QwtPlotMagnifier to zoom around the current
//cursor position as apposed to the default behaviour of zooming around
//the middle of the plot canvas.

//System includes

//Library includes
#include <qwt_plot.h>

//Local includes
#include "CursorCentredQwtPlotMagnifier.h"

cCursorCentredQwtPlotMagnifier::cCursorCentredQwtPlotMagnifier(QWidget* pCanvas) :
    QwtPlotMagnifier(pCanvas)
{
}

void cCursorCentredQwtPlotMagnifier::widgetWheelEvent(QWheelEvent* pWheelEvent)
{
    //Store the current mouse position
    m_oMousePosition = pWheelEvent->pos();

    //Execute the default functionality of the base class
    QwtPlotMagnifier::widgetWheelEvent(pWheelEvent);
}

void cCursorCentredQwtPlotMagnifier::rescale(double dFactor)
{
    //Extended from default Qwt Code:

    QwtPlot* plt = plot();
    if ( plt == NULL )
        return;

    dFactor = qAbs( dFactor );
    if ( dFactor == 1.0 || dFactor == 0.0 )
        return;

    bool doReplot = false;

    const bool autoReplot = plt->autoReplot();
    plt->setAutoReplot( false );

    for ( int axisId = 0; axisId < QwtPlot::axisCnt; axisId++ )
    {
        if ( isAxisEnabled( axisId ) )
        {
            const QwtScaleMap scaleMap = plt->canvasMap( axisId );

            double v1 = scaleMap.s1();
            double v2 = scaleMap.s2();

            double dMousePosition;

            if ( scaleMap.transformation() )
            {
                // the coordinate system of the paint device is always linear

                v1 = scaleMap.transform( v1 ); // scaleMap.p1()
                v2 = scaleMap.transform( v2 ); // scaleMap.p2()
            }

            if(axisId == QwtPlot::yLeft || axisId == QwtPlot::yRight)
            {
                dMousePosition = scaleMap.invTransform( m_oMousePosition.y() );
            }
            if(axisId == QwtPlot::xBottom || axisId == QwtPlot::xBottom)
            {
                dMousePosition = scaleMap.invTransform( m_oMousePosition.x() );
            }

            double dLowerDistance = dMousePosition - v1;
            double dUpperDistance = v2 - dMousePosition;

            v1 = dMousePosition - dLowerDistance * dFactor;
            v2 = dMousePosition + dUpperDistance * dFactor;

            if ( scaleMap.transformation() )
            {
                v1 = scaleMap.invTransform( v1 );
                v2 = scaleMap.invTransform( v2 );
            }

            plt->setAxisScale( axisId, v1, v2 );
            doReplot = true;
        }
    }

    plt->setAutoReplot( autoReplot );

    if ( doReplot )
        plt->replot();
}
