#ifndef RIVEQT_RIVEQTPAINT_HPP
#define RIVEQT_RIVEQTPAINT_HPP

#include <QBrush>
#include <QPen>

class RiveQtPaint
{
public:
    RiveQtPaint();

    const QBrush& brush() const { return m_Brush; }
    const QPen& pen() const { return m_Pen; }

private:
    QBrush m_Brush;
    QPen m_Pen;
};

#endif // RIVEQT_RIVEQTPAINT_HPP
