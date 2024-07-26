/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/style.h>

#include <gt_utilities.h>

#include <random>

using namespace intelli;
using namespace intelli::style;

namespace intelli
{

// forward declarations
class ByteArrayData;
class StringData;
class DoubleData;
class IntData;
class BoolData;
class FileData;
class ObjectData;

} // namespace intelli

/// registered styles
static auto& styles()
{
    static std::map<QString, StyleData> styles;

    static auto initOnce = [](){

        StyleData style;

        /// custom type colors
        auto& ctc = style.connection.customTypeColors;
        ctc.insert(GT_LOG_TO_STR(intelli::ByteArrayData), QColor::fromHsv(195, 240, 255));
        ctc.insert(GT_LOG_TO_STR(intelli::StringData), QColor::fromHsv(210, 240, 255));

        ctc.insert(GT_LOG_TO_STR(intelli::DoubleData), QColor::fromHsv(270, 130, 240));
        ctc.insert(GT_LOG_TO_STR(intelli::IntData), QColor::fromHsv(285, 200, 220));
        ctc.insert(GT_LOG_TO_STR(intelli::BoolData), QColor::fromHsv(180, 200, 240));

        ctc.insert(GT_LOG_TO_STR(intelli::FileData), QColor::fromHsv(30, 240, 200));
        ctc.insert(GT_LOG_TO_STR(intelli::ObjectData), QColor::fromHsv(100, 170, 240));

        /// dark
        style.id = styleId(DefaultStyle::Dark);

        style.view.background = QColor{21, 38, 53};
        style.view.gridline = QColor{25, 25, 25, 255};

        style.node.background = QColor{36, 49, 63};
        style.node.defaultOutline = QColor{63, 73, 86};
        style.node.selectedOutline = QColor{255, 165, 0};
        style.node.hoveredOutline = style.node.defaultOutline;
        style.node.compatiblityTintModifier = 20;

        style.connection.defaultOutline = style.node.defaultOutline;
        style.connection.selectedOutline = style.node.selectedOutline;
        style.connection.hoveredOutline = Qt::lightGray;
        style.connection.inactiveOutline = Qt::gray;

        styles.emplace(style.id, style);

        /// bright
        style.id = styleId(DefaultStyle::Bright);

        style.view.background = QColor{255, 255, 255};
        style.view.gridline = QColor{200, 200, 255, 125};

        style.node.background = QColor{245, 245, 245};
        style.node.defaultOutline = QColor{"darkgray"};
        style.node.selectedOutline = QColor{"deepskyblue"};
        style.node.hoveredOutline = style.node.defaultOutline;

        style.connection.defaultOutline = style.node.defaultOutline;
        style.connection.selectedOutline = style.node.selectedOutline;

        styles.emplace(style.id, style);

        return 0;
    }();
    Q_UNUSED(initOnce);

    return styles;
}

static StyleData& styleInstance()
{
    static StyleData& currentStyle = []() -> StyleData& {
        return (styles().begin()->second);
    }();
    return currentStyle;
}

QColor
StyleData::ConnectionData::typeColor(TypeId const& typeId) const
{
    if (typeId.isEmpty() || !useCustomTypeColors)
    {
        return defaultOutline;
    }

    auto iter = customTypeColors.find(typeId);
    if (iter != customTypeColors.end())
    {
        return *iter;
    }

    if (!generateMissingTypeColors)
    {
        return defaultOutline;
    }

    return generateTypeColor(typeId);
}

void
intelli::style::applyStyle(StyleId const& id)
{
    if (StyleData const* style = findStyle(id)) styleInstance() = *style;
}

void
intelli::style::applyStyle(DefaultStyle style)
{
    applyStyle(styleId(style));
}

void
intelli::style::registerStyle(StyleId const& id, StyleData style, bool apply)
{
    if (id.isEmpty()) return;

    if (!findStyle(id))
    {
        style.id = id;
        styles().insert({id, std::move(style)});
    }

    if (apply) applyStyle(id);
}

StyleData const&
intelli::style::currentStyle()
{
    return styleInstance();
}

StyleData const*
intelli::style::findStyle(StyleId const& id)
{
    auto const& styles = ::styles();
    auto iter = styles.find(id);
    if (iter == styles.end()) return nullptr;

    return &(iter->second);
}

StyleData const*
intelli::style::findStyle(DefaultStyle style)
{
    return findStyle(styleId(style));
}

StyleId const&
intelli::style::styleId(DefaultStyle theme)
{
    static auto bright = QStringLiteral("DefaultBright");
    static auto dark = QStringLiteral("DefaultDark");

    switch (theme)
    {
    default:
        gtError().medium()
            << QObject::tr("Unknown default style '%1'").arg((int)theme);
    case DefaultStyle::Bright:
        return bright;
    case DefaultStyle::Dark:
        return dark;
    }
}

QVector<StyleId>
intelli::style::registeredStyles()
{
    QVector<StyleId> ids;
    std::transform(styles().begin(), styles().end(), std::back_inserter(ids),
                   [](auto const& iter){
        return iter.first;
    });
    return ids;
}

QColor
intelli::style::tint(QColor const& color, int r, int g, int b)
{
    constexpr int MAX = std::numeric_limits<uint8_t>::max();
    constexpr int MIN = std::numeric_limits<uint8_t>::min();

    return QColor{
        gt::clamp(color.red()   + r, MIN, MAX),
        gt::clamp(color.green() + g, MIN, MAX),
        gt::clamp(color.blue()  + b, MIN, MAX)
    };
}

QColor
intelli::style::invert(QColor const& color)
{
    constexpr int MAX = std::numeric_limits<uint8_t>::max();

    return QColor{MAX - color.red(), MAX - color.green(), MAX - color.blue()};
}

QColor
intelli::style::generateTypeColor(TypeId const& typeId)
{
    static QHash<QString, QColor> cache;

    auto iter = cache.constFind(typeId);
    if (iter != cache.constEnd())
    {
        return *iter;
    }

    // from QtNodes
    std::size_t hash = qHash(typeId);

    std::size_t const hue_range = 0xFF;

    std::mt19937 gen(static_cast<unsigned int>(hash));
    std::uniform_int_distribution<int> distrib(0, hue_range);

    int hue = distrib(gen);
    int sat = 120 + hash % 129;

    QColor color = QColor::fromHsl(hue, sat, 160);
    cache.insert(typeId, color);
    return color;
}
