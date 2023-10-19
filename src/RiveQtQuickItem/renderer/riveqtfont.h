
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QRawFont>
#include <QGlyphRun>
#include <QTextLayout>
#include <QTextOption>

#include <rive/text_engine.hpp>
#include <rive/core.hpp>

class RiveQtFont : public rive::Font, public std::enable_shared_from_this<RiveQtFont>
{

public:
    RiveQtFont(const QFont &font, const QFontMetricsF &fontMetrics);

    RiveQtFont(const QFont &font, const std::vector<rive::Font::Coord> &coords);

    const QFont &font() const { return m_font; }

    uint32_t tagFromName(const char *name) const;

    rive::Font::Axis getAxis(uint16_t index) const override;

    uint16_t getAxisCount() const override;
    //std::vector<rive::Font::Coord> getCoords() const override;

    //rive::rcp<rive::Font> makeAtCoords(rive::Span<const rive::Font::Coord> coords) const override;

    float getAxisValue(uint32_t axisTag) const override { return 0.0f; }
    rive::SimpleArray<uint32_t> features() const override { return rive::SimpleArray<uint32_t>(); }
    bool hasGlyph(rive::Span<const rive::Unichar>) const override { return false; }
    uint32_t getFeatureValue(uint32_t featureTag) const override { return 0; }

    rive::rcp<rive::Font> withOptions(rive::Span<const rive::Font::Coord> variableAxes,
                                      rive::Span<const rive::Font::Feature> features) const override { return nullptr; } 

    rive::RawPath getPath(rive::GlyphID glyphId) const override;

    rive::SimpleArray<rive::Paragraph> onShapeText(rive::Span<const rive::Unichar> text,
                                                   rive::Span<const rive::TextRun> runs) const override;

private:
    rive::Font::LineMetrics calculateLineMetrics(const QFont &font);

    QFont m_font;
    std::vector<rive::Font::Coord> m_coords;
};
