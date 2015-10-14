#ifndef BASIC_QWT_LINE_PLOT_POSITION_PICKER_H
#define BASIC_QWT_LINE_PLOT_POSITION_PICKER_H

//System includes

//Library includes
#include <qwt_plot_picker.h>
#include <QString>

//Local includes

class cBasicQwtLinePlotPositionPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
    explicit cBasicQwtLinePlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget* pCanvas);

    //Overload this function to return the tracker text with our units of choice
    virtual QwtText trackerTextF(const QPointF &oPosition) const;

    void    setXUnit(const QString &qstrXUnit);
    void    setYUnit(const QString &qstrYUnit);

protected:
    QString m_qstrXUnit;
    QString m_qstrYUnit;

signals:

public slots:

};

#endif // BASIC_QWT_LINE_PLOT_POSITION_PICKER_H
