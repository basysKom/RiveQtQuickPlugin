// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QPainterPath>

#include "renderer/riveqtfont.h"

RiveQtFont::RiveQtFont(const QFont &font, const QFontMetricsF &fontMetrics)
    : rive::Font({ static_cast<float>(fontMetrics.ascent()), static_cast<float>(fontMetrics.descent()) })
    , m_font(font)
{
}

RiveQtFont::RiveQtFont(const QFont &font, const std::vector<Coord> &coords)
    : rive::Font(calculateLineMetrics(font))
    , m_font(font)
    , m_coords(coords)
{
}

uint32_t RiveQtFont::tagFromName(const char *name) const
{
    uint32_t tag = 0;
    for (int i = 0; i < 4 && name[i] != '\0'; i++) {
        tag |= (name[i] << (8 * (3 - i)));
    }
    return tag;
}

rive::Font::Axis RiveQtFont::getAxis(uint16_t index) const
{
    // QFontDatabase db;
    QRawFont rawFont = QRawFont::fromFont(m_font);
    QByteArray fontData = rawFont.fontTable("head");

    // TODO: IMPLEMENT?

    return { 0, 0, 0, 0 };
}

uint16_t RiveQtFont::getAxisCount() const
{
    // QFontDatabase db;
    QRawFont rawFont = QRawFont::fromFont(m_font);
    QByteArray fontData = rawFont.fontTable("head");

    // TODO: IMPLEMENT?

    return 0;
}

/*std::vector<rive::Font::Coord> RiveQtFont::getCoords() const
{
    // QFontDatabase db;
    QRawFont rawFont = QRawFont::fromFont(m_font);
    QByteArray fontData = rawFont.fontTable("head");

    // TODO: IMPLEMENT?

    return {};
}*/

/*rive::rcp<rive::Font> RiveQtFont::makeAtCoords(rive::Span<const Coord> coords) const
{
    std::vector<rive::Font::Coord> coordVec(coords.begin(), coords.end());
    return rive::rcp<RiveQtFont>(new RiveQtFont(m_font, coordVec));
}*/

rive::RawPath RiveQtFont::getPath(rive::GlyphID glyphId) const
{
    // Get the raw font from the QFont.
    QRawFont rawFont = QRawFont::fromFont(m_font);

    // Get the QPainterPath for the glyph.
    QPainterPath glyphPath = rawFont.pathForGlyph(glyphId);

    // Convert the QPainterPath to a rive::RawPath.
    rive::RawPath rivePath;
    for (int i = 0; i < glyphPath.elementCount(); i++) {
        QPainterPath::Element element = glyphPath.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:
            rivePath.moveTo(element.x, element.y);
            break;
        case QPainterPath::LineToElement:
            rivePath.lineTo(element.x, element.y);
            break;
        case QPainterPath::CurveToElement: {
            QPointF cp1 = glyphPath.elementAt(i).operator QPointF();
            QPointF cp2 = glyphPath.elementAt(i + 1).operator QPointF();
            QPointF endPoint = glyphPath.elementAt(i + 2).operator QPointF();
            rivePath.cubicTo(cp1.x(), cp1.y(), cp2.x(), cp2.y(), endPoint.x(), endPoint.y());
            i += 2;
        } break;
        default:
            break;
        }
    }

    return rivePath;
}

rive::SimpleArray<rive::Paragraph> RiveQtFont::onShapeText(rive::Span<const rive::Unichar> text, rive::Span<const rive::TextRun> runs,
                                                           int textDirectionFlag) const
{
    // TODO: support text direction Flag
    Q_UNUSED(textDirectionFlag);

    QString qText = QString::fromUcs4(reinterpret_cast<const char32_t *>(text.begin()), text.size());
    std::vector<rive::Paragraph> tempParagraphs;

    QTextLayout layout(qText, m_font);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    while (line.isValid()) {
        std::vector<rive::GlyphRun> glyphRuns;

        int textStart = line.textStart();
        int textLength = line.textLength();
        QList<QGlyphRun> qGlyphRuns = layout.glyphRuns(textStart, textLength);

        for (const auto &run : runs) {
            for (const auto &qGlyphRun : qGlyphRuns) {
                QVector<quint32> glyphIndexes = qGlyphRun.glyphIndexes();
                QVector<QPointF> glyphPositions = qGlyphRun.positions();

                rive::GlyphRun riveGlyphRun(glyphIndexes.size());
                riveGlyphRun.font = run.font;
                riveGlyphRun.size = run.size;

                for (int i = 0; i < glyphIndexes.size(); i++) {
                    rive::GlyphID glyphID = static_cast<rive::GlyphID>(glyphIndexes[i]);
                    riveGlyphRun.glyphs[i] = glyphID;
                    riveGlyphRun.textIndices[i] = i; // Assuming one glyph per character
                    riveGlyphRun.advances[i] = glyphPositions[i].x();
                    riveGlyphRun.xpos[i] = glyphPositions[i].x();
                }
                riveGlyphRun.xpos[glyphIndexes.size()] = line.horizontalAdvance(); // Rightmost extent of the last glyph

                // TODO: Fill in other fields such as breaks, styleId, and dir as needed

                glyphRuns.push_back(riveGlyphRun);
            }
        }

        tempParagraphs.push_back(rive::Paragraph { glyphRuns });
        line = layout.createLine();
    }
    layout.endLayout();

    rive::SimpleArray<rive::Paragraph> paragraphs(tempParagraphs.size());
    std::copy(std::make_move_iterator(tempParagraphs.begin()), std::make_move_iterator(tempParagraphs.end()), paragraphs.begin());
    return paragraphs;
}

rive::Font::LineMetrics RiveQtFont::calculateLineMetrics(const QFont &font)
{
    QFontMetricsF metrics(font);
    rive::Font::LineMetrics lm;
    lm.ascent = static_cast<float>(metrics.ascent());
    lm.descent = static_cast<float>(metrics.descent());
    return lm;
}
