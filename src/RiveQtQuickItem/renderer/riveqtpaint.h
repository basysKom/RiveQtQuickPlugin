// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

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
