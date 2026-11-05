/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include "test_helper.h"

#include <intelli/gui/graphselectionaction.h>
#include <intelli/private/graphselectionactionmenu.h>

#include <QApplication>
#include <QMenu>
#include <QIconEngine>
#include <QLibrary>
#include <QObject>
#include <QPainter>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <memory>

using namespace intelli;

namespace
{

QAction*
findAction(QMenu& menu, QString const& text)
{
    auto const& actions = menu.actions();
    auto iter = std::find_if(actions.begin(), actions.end(), [&text](QAction* action) {
        return action->text() == text;
    });
    return iter == actions.end() ? nullptr : *iter;
}

class CountingIconEngine final : public QIconEngine
{
public:
    explicit CountingIconEngine(int& liveEngines) : m_liveEngines{&liveEngines}
    {
        ++*m_liveEngines;
    }

    ~CountingIconEngine() override
    {
        --*m_liveEngines;
    }

    QIconEngine* clone() const override
    {
        return new CountingIconEngine{*m_liveEngines};
    }

    void paint(QPainter* painter,
               QRect const& rect,
               QIcon::Mode,
               QIcon::State) override
    {
        painter->fillRect(rect, Qt::red);
    }

private:
    int* m_liveEngines;
};

} // namespace

TEST(GraphSelectionActionRegistry,
     owner_destruction_unregisters_action_and_invalidates_snapshots)
{
    Graph graph;
    bool triggered = false;
    auto owner = std::make_unique<QObject>();

    GraphSelectionAction action;
    action.id = QStringLiteral("owner-lifetime");
    action.text = QStringLiteral("Owner lifetime action");
    action.isVisible = [](Graph const&, QVector<ObjectUuid> const&) {
        return true;
    };
    action.isEnabled = [](Graph const&, QVector<ObjectUuid> const&) {
        return true;
    };
    action.trigger = [&triggered](Graph&, QVector<ObjectUuid> const&, QWidget*) {
        triggered = true;
    };

    registerGraphSelectionAction(*owner, std::move(action));

    auto snapshots = graphSelectionActions();
    auto snapshot = std::find_if(snapshots.begin(), snapshots.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("owner-lifetime");
    });
    ASSERT_NE(snapshot, snapshots.end());
    ASSERT_TRUE(snapshot->trigger);
    ASSERT_TRUE(snapshot->isVisible);
    ASSERT_TRUE(snapshot->isEnabled);

    owner.reset();

    auto remaining = graphSelectionActions();
    EXPECT_EQ(std::count_if(remaining.begin(), remaining.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("owner-lifetime");
    }), 0);

    snapshot->trigger(graph, {}, nullptr);
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(snapshot->isVisible(graph, {}));
    EXPECT_FALSE(snapshot->isEnabled(graph, {}));
}

TEST(GraphSelectionActionRegistry,
     duplicate_id_replaces_registration_and_transfers_owner_lifetime)
{
    Graph graph;
    int oldTriggerCount = 0;
    int newTriggerCount = 0;
    auto oldOwner = std::make_unique<QObject>();
    auto newOwner = std::make_unique<QObject>();

    GraphSelectionAction oldAction;
    oldAction.id = QStringLiteral("replaceable");
    oldAction.text = QStringLiteral("Old action");
    oldAction.trigger = [&oldTriggerCount](Graph&, QVector<ObjectUuid> const&, QWidget*) {
        ++oldTriggerCount;
    };
    registerGraphSelectionAction(*oldOwner, std::move(oldAction));

    auto originalActions = graphSelectionActions();
    auto originalSnapshot = std::find_if(
        originalActions.begin(), originalActions.end(), [](auto const& entry) {
            return entry.id == QStringLiteral("replaceable");
        });
    ASSERT_NE(originalSnapshot, originalActions.end());

    GraphSelectionAction newAction;
    newAction.id = QStringLiteral("replaceable");
    newAction.text = QStringLiteral("Replacement action");
    newAction.trigger = [&newTriggerCount](Graph&, QVector<ObjectUuid> const&, QWidget*) {
        ++newTriggerCount;
    };
    registerGraphSelectionAction(*newOwner, std::move(newAction));

    auto actions = graphSelectionActions();
    auto replacement = std::find_if(actions.begin(), actions.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("replaceable");
    });
    ASSERT_NE(replacement, actions.end());
    EXPECT_EQ(std::count_if(actions.begin(), actions.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("replaceable");
    }), 1);
    EXPECT_EQ(replacement->text, QStringLiteral("Replacement action"));

    originalSnapshot->trigger(graph, {}, nullptr);
    EXPECT_EQ(oldTriggerCount, 0);

    oldOwner.reset();

    replacement->trigger(graph, {}, nullptr);
    EXPECT_EQ(oldTriggerCount, 0);
    EXPECT_EQ(newTriggerCount, 1);

    newOwner.reset();

    auto remaining = graphSelectionActions();
    EXPECT_EQ(std::count_if(remaining.begin(), remaining.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("replaceable");
    }), 0);
}

TEST(GraphSelectionActionRegistry,
     snapshot_preserves_mutable_callback_state_between_invocations)
{
    Graph graph;
    QObject owner;
    QVector<int> observedCounts;

    GraphSelectionAction action;
    action.id = QStringLiteral("stateful-callback");
    action.text = QStringLiteral("Stateful callback");
    action.trigger = [&observedCounts, count = 0](
            Graph&, QVector<ObjectUuid> const&, QWidget*) mutable {
        observedCounts.push_back(++count);
    };
    registerGraphSelectionAction(owner, std::move(action));

    auto actions = graphSelectionActions();
    auto snapshot = std::find_if(actions.begin(), actions.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("stateful-callback");
    });
    ASSERT_NE(snapshot, actions.end());
    ASSERT_TRUE(snapshot->trigger);

    snapshot->trigger(graph, {}, nullptr);
    snapshot->trigger(graph, {}, nullptr);

    EXPECT_EQ(observedCounts, (QVector<int>{1, 2}));
}

TEST(GraphSelectionActionRegistry,
     cascading_owner_destruction_is_safe_during_unregistration)
{
    auto firstOwner = std::make_unique<QObject>();
    auto dependentOwner = std::make_shared<QObject>();
    std::weak_ptr<QObject> dependentOwnerGuard = dependentOwner;

    GraphSelectionAction firstAction;
    firstAction.id = QStringLiteral("cascade-first");
    firstAction.text = QStringLiteral("Cascade first");
    firstAction.trigger = [dependentOwner](
            Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(*firstOwner, std::move(firstAction));

    GraphSelectionAction dependentAction;
    dependentAction.id = QStringLiteral("cascade-dependent");
    dependentAction.text = QStringLiteral("Cascade dependent");
    dependentAction.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(*dependentOwner, std::move(dependentAction));
    dependentOwner.reset();

    firstOwner.reset();

    EXPECT_TRUE(dependentOwnerGuard.expired());
    auto remaining = graphSelectionActions();
    EXPECT_TRUE(std::none_of(remaining.begin(), remaining.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("cascade-first") ||
               entry.id == QStringLiteral("cascade-dependent");
    }));
}

TEST(GraphSelectionActionRegistry,
     cascading_owner_destruction_is_safe_during_duplicate_replacement)
{
    QObject originalOwner;
    QObject replacementOwner;
    auto dependentOwner = std::make_shared<QObject>();
    std::weak_ptr<QObject> dependentOwnerGuard = dependentOwner;

    GraphSelectionAction original;
    original.id = QStringLiteral("cascade-replacement");
    original.text = QStringLiteral("Cascade original");
    original.trigger = [dependentOwner](
            Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(originalOwner, std::move(original));

    GraphSelectionAction dependent;
    dependent.id = QStringLiteral("cascade-replacement-dependent");
    dependent.text = QStringLiteral("Cascade replacement dependent");
    dependent.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(*dependentOwner, std::move(dependent));
    dependentOwner.reset();

    GraphSelectionAction replacement;
    replacement.id = QStringLiteral("cascade-replacement");
    replacement.text = QStringLiteral("Cascade replacement");
    replacement.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(replacementOwner, std::move(replacement));

    EXPECT_TRUE(dependentOwnerGuard.expired());
    auto remaining = graphSelectionActions();
    EXPECT_EQ(std::count_if(remaining.begin(), remaining.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("cascade-replacement");
    }), 1);
    EXPECT_TRUE(std::none_of(remaining.begin(), remaining.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("cascade-replacement-dependent");
    }));
}

TEST(GraphSelectionActionRegistry,
     snapshot_owns_string_metadata_independently_of_registration_storage)
{
    QObject owner;
    QChar idStorage[] = {u'r', u'a', u'w', u'-', u'i', u'd'};
    QChar textStorage[] = {u'R', u'a', u'w', u' ', u't', u'e', u'x', u't'};

    GraphSelectionAction action;
    action.id = QString::fromRawData(idStorage, std::size(idStorage));
    action.text = QString::fromRawData(textStorage, std::size(textStorage));
    action.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(owner, std::move(action));

    auto snapshots = graphSelectionActions();
    auto snapshot = std::find_if(snapshots.begin(), snapshots.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("raw-id");
    });
    ASSERT_NE(snapshot, snapshots.end());

    std::fill(std::begin(idStorage), std::end(idStorage), u'x');
    std::fill(std::begin(textStorage), std::end(textStorage), u'x');

    EXPECT_EQ(snapshot->id, QStringLiteral("raw-id"));
    EXPECT_EQ(snapshot->text, QStringLiteral("Raw text"));
}

TEST(GraphSelectionActionRegistry,
     snapshot_does_not_retain_registration_icon_engine)
{
    auto owner = std::make_unique<QObject>();
    int liveIconEngines = 0;

    GraphSelectionAction action;
    action.id = QStringLiteral("custom-icon-engine");
    action.text = QStringLiteral("Custom icon engine");
    action.icon = QIcon{new CountingIconEngine{liveIconEngines}};
    action.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(*owner, std::move(action));
    ASSERT_EQ(liveIconEngines, 1);

    auto snapshots = graphSelectionActions();
    auto snapshot = std::find_if(snapshots.begin(), snapshots.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("custom-icon-engine");
    });
    ASSERT_NE(snapshot, snapshots.end());
    ASSERT_FALSE(snapshot->icon.isNull());

    owner.reset();

    EXPECT_EQ(liveIconEngines, 0);
    EXPECT_FALSE(snapshot->icon.pixmap(QSize{16, 16}).isNull());
}

TEST(GraphSelectionActionRegistry,
     retained_snapshot_is_safe_after_registration_module_unloads)
{
    QLibrary plugin{QString::fromUtf8(GRAPH_SELECTION_ACTION_TEST_PLUGIN_PATH)};
    ASSERT_TRUE(plugin.load()) << qPrintable(plugin.errorString());

    using RegisterAction = void (*)(QObject*);
    auto registerAction = reinterpret_cast<RegisterAction>(
        plugin.resolve("registerGraphSelectionActionTestPlugin"));
    ASSERT_TRUE(registerAction) << qPrintable(plugin.errorString());

    auto owner = std::make_unique<QObject>();
    registerAction(owner.get());

    auto snapshots = graphSelectionActions();
    auto snapshot = std::find_if(snapshots.begin(), snapshots.end(), [](auto const& entry) {
        return entry.id == QStringLiteral("dynamic-plugin-action");
    });
    ASSERT_NE(snapshot, snapshots.end());
    ASSERT_FALSE(snapshot->icon.isNull());

    owner.reset();
    ASSERT_TRUE(plugin.unload()) << qPrintable(plugin.errorString());

    EXPECT_EQ(snapshot->id, QStringLiteral("dynamic-plugin-action"));
    EXPECT_EQ(snapshot->text, QStringLiteral("Dynamic plugin action"));
    EXPECT_FALSE(snapshot->icon.pixmap(QSize{16, 16}).isNull());

    Graph graph;
    ASSERT_TRUE(snapshot->trigger);
    snapshot->trigger(graph, {}, nullptr);
}

TEST(GraphSelectionActionMenu,
     applies_predicates_normalizes_selection_and_passes_menu_parent)
{
    Graph graph;
    QWidget viewParent;
    QMenu menu{&viewParent};

    QObject owner;
    Graph const* predicateGraph = nullptr;
    QVector<ObjectUuid> predicateSelection;
    Graph* triggeredGraph = nullptr;
    QVector<ObjectUuid> triggeredSelection;
    QWidget* triggerParent = nullptr;

    GraphSelectionAction hidden;
    hidden.id = QStringLiteral("menu-hidden");
    hidden.text = QStringLiteral("External hidden action");
    hidden.isVisible = [](Graph const&, QVector<ObjectUuid> const&) {
        return false;
    };
    hidden.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(owner, std::move(hidden));

    GraphSelectionAction disabled;
    disabled.id = QStringLiteral("menu-disabled");
    disabled.text = QStringLiteral("External disabled action");
    disabled.isEnabled = [](Graph const&, QVector<ObjectUuid> const&) {
        return false;
    };
    disabled.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(owner, std::move(disabled));

    GraphSelectionAction enabled;
    enabled.id = QStringLiteral("menu-enabled");
    enabled.text = QStringLiteral("External enabled action");
    enabled.isVisible = [&predicateGraph, &predicateSelection](
            Graph const& selectedGraph, QVector<ObjectUuid> const& selection) {
        predicateGraph = &selectedGraph;
        predicateSelection = selection;
        return true;
    };
    enabled.isEnabled = [](Graph const&, QVector<ObjectUuid> const&) {
        return true;
    };
    enabled.trigger = [&triggeredGraph, &triggeredSelection, &triggerParent](
            Graph& selectedGraph,
            QVector<ObjectUuid> const& selection,
            QWidget* parent) {
        triggeredGraph = &selectedGraph;
        triggeredSelection = selection;
        triggerParent = parent;
    };
    registerGraphSelectionAction(owner, std::move(enabled));

    detail::addGraphSelectionActionsToMenu(
        menu, graph, {B_uuid, A_uuid, B_uuid, {}});

    EXPECT_EQ(menu.parentWidget(), &viewParent);
    EXPECT_FALSE(findAction(menu, QStringLiteral("External hidden action")));

    auto* disabledAction = findAction(menu, QStringLiteral("External disabled action"));
    ASSERT_TRUE(disabledAction);
    EXPECT_FALSE(disabledAction->isEnabled());

    auto* enabledAction = findAction(menu, QStringLiteral("External enabled action"));
    ASSERT_TRUE(enabledAction);
    EXPECT_TRUE(enabledAction->isEnabled());
    EXPECT_EQ(predicateGraph, &graph);
    ASSERT_EQ(predicateSelection.size(), 2);
    EXPECT_EQ(predicateSelection.at(0), A_uuid);
    EXPECT_EQ(predicateSelection.at(1), B_uuid);

    enabledAction->trigger();

    EXPECT_EQ(triggeredGraph, &graph);
    ASSERT_EQ(triggeredSelection.size(), 2);
    EXPECT_EQ(triggeredSelection.at(0), A_uuid);
    EXPECT_EQ(triggeredSelection.at(1), B_uuid);
    EXPECT_EQ(triggerParent, &viewParent);
}

TEST(GraphSelectionActionMenu,
     parent_destruction_while_menu_is_active_deletes_menu_safely)
{
    auto* viewParent = new QWidget;
    auto menu = detail::createGraphSelectionMenu(viewParent);
    ASSERT_TRUE(menu);
    EXPECT_EQ(menu->parentWidget(), viewParent);
    menu->addAction(QStringLiteral("Keep menu active"));

    QTimer::singleShot(0, qApp, [viewParent]() {
        delete viewParent;
    });

    EXPECT_EQ(menu->exec(QPoint{10, 10}), static_cast<QAction*>(nullptr));
    EXPECT_TRUE(menu.isNull());
}

TEST(GraphSelectionActionMenu,
     parent_destruction_from_predicate_stops_menu_population_safely)
{
    Graph graph;
    QObject owner;
    auto* viewParent = new QWidget;
    auto menu = detail::createGraphSelectionMenu(viewParent);
    ASSERT_TRUE(menu);
    int postDestructionPredicateCalls = 0;

    GraphSelectionAction action;
    action.id = QStringLiteral("destroy-menu-from-predicate");
    action.text = QStringLiteral("Destroy menu from predicate");
    action.isVisible = [viewParent](Graph const&, QVector<ObjectUuid> const&) {
        delete viewParent;
        return true;
    };
    action.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(owner, std::move(action));

    GraphSelectionAction laterAction;
    laterAction.id = QStringLiteral("predicate-after-menu-destruction");
    laterAction.text = QStringLiteral("Predicate after menu destruction");
    laterAction.isVisible = [&postDestructionPredicateCalls](
            Graph const&, QVector<ObjectUuid> const&) {
        ++postDestructionPredicateCalls;
        return true;
    };
    laterAction.trigger = [](Graph&, QVector<ObjectUuid> const&, QWidget*) {};
    registerGraphSelectionAction(owner, std::move(laterAction));

    EXPECT_FALSE(detail::addGraphSelectionActionsToMenu(*menu, graph, {}));

    EXPECT_TRUE(menu.isNull());
    EXPECT_EQ(postDestructionPredicateCalls, 0);
}
