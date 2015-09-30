#ifndef BASIC_QWT_LINE_PLOT_PICKER_H
#define BASIC_QWT_LINE_PLOT_PICKER_H

//System includes

//Library includes
#include <qwt_plot_picker.h>
#include <QString>

//Local includes

class cBasicQwtLinePlotPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
    explicit cBasicQwtLinePlotPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget* pCanvas);

    //Overload this function to return the tracking with our units of choice
    //QwtText trackerText(const QPoint &oPosition) const;
    QwtText trackerTextF(const QPointF &oPosition) const;

    void    setXUnit(const QString &qstrXUnit);
    void    setYUnit(const QString &qstrYUnit);

private:
    QString m_qstrXUnit;
    QString m_qstrYUnit;

signals:

public slots:

};

#endif // BASIC_QWT_LINE_PLOT_PICKER_H
