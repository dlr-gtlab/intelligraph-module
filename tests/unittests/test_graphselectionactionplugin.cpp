/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/graphselectionaction.h>

#include <QIconEngine>
#include <QObject>
#include <QPainter>

#include <utility>

namespace
{

class ModuleIconEngine final : public QIconEngine
{
public:
    QIconEngine* clone() const override
    {
        return new ModuleIconEngine;
    }

    void paint(QPainter* painter,
               QRect const& rect,
               QIcon::Mode,
               QIcon::State) override
    {
        painter->fillRect(rect, Qt::green);
    }
};

} // namespace

extern "C" Q_DECL_EXPORT void
registerGraphSelectionActionTestPlugin(QObject* owner)
{
    intelli::GraphSelectionAction action;
    action.id = QStringLiteral("dynamic-plugin-action");
    action.text = QStringLiteral("Dynamic plugin action");
    action.icon = QIcon{new ModuleIconEngine};
    action.trigger = [](intelli::Graph&,
                        QVector<intelli::ObjectUuid> const&,
                        QWidget*) {};
    intelli::registerGraphSelectionAction(*owner, std::move(action));
}
