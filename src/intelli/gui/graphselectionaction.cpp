/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include "intelli/gui/graphselectionaction.h"
#include "intelli/private/graphselectionactionmenu.h"

#include <intelli/graph.h>

#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QWidget>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

using namespace intelli;

namespace
{

using RegistrationId = quint64;

struct RegisteredAction
{
    RegistrationId registrationId = 0;
    QPointer<QObject> owner;
    GraphSelectionAction action;
    QMetaObject::Connection ownerDestroyed;
};

using RegisteredActionPtr = std::shared_ptr<RegisteredAction>;

QVector<RegisteredActionPtr>&
registeredActions()
{
    static QVector<RegisteredActionPtr> actions;
    return actions;
}

RegistrationId
nextRegistrationId()
{
    static RegistrationId next = 0;
    return ++next;
}

void
assertApplicationThread(char const* function)
{
    auto* application = QCoreApplication::instance();
    Q_ASSERT_X(application, function, "an application instance is required");
    Q_ASSERT_X(QThread::currentThread() == application->thread(),
               function,
               "graph selection actions are restricted to the application thread");
}

RegisteredActionPtr
findRegistration(RegistrationId id)
{
    assertApplicationThread("findRegistration");
    auto& actions = registeredActions();
    auto iter = std::find_if(actions.begin(), actions.end(), [id](auto const& entry) {
        return entry->registrationId == id && entry->owner;
    });
    return iter == actions.end() ? RegisteredActionPtr{} : *iter;
}

void
removeRegistration(RegistrationId id)
{
    assertApplicationThread("removeRegistration");
    auto& actions = registeredActions();
    auto iter = std::find_if(actions.begin(), actions.end(), [id](auto const& entry) {
        return entry->registrationId == id;
    });
    if (iter == actions.end()) return;

    auto retired = std::move(*iter);
    actions.erase(iter);
    QObject::disconnect(retired->ownerDestroyed);
}

QString
owningCopy(QString const& value)
{
    return value.isNull() ? QString{} : QString{value.constData(), value.size()};
}

QIcon
hostOwnedIcon(QIcon const& icon)
{
    if (icon.isNull()) return {};

    QIcon copy;
    constexpr std::array modes{
        QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected};
    constexpr std::array states{QIcon::Off, QIcon::On};
    for (auto mode : modes)
    {
        for (auto state : states)
        {
            auto pixmap = icon.pixmap(QSize{16, 16}, mode, state);
            if (!pixmap.isNull()) copy.addPixmap(std::move(pixmap), mode, state);
        }
    }
    return copy;
}

GraphSelectionAction
makeSnapshot(RegisteredActionPtr const& registration)
{
    auto const id = registration->registrationId;

    GraphSelectionAction snapshot;
    snapshot.id = owningCopy(registration->action.id);
    snapshot.text = owningCopy(registration->action.text);
    snapshot.icon = hostOwnedIcon(registration->action.icon);

    snapshot.isVisible = [id](Graph const& graph, QVector<ObjectUuid> const& selection) {
        auto current = findRegistration(id);
        if (!current) return false;
        auto& callback = current->action.isVisible;
        return callback ? callback(graph, selection) : true;
    };

    snapshot.isEnabled = [id](Graph const& graph, QVector<ObjectUuid> const& selection) {
        auto current = findRegistration(id);
        if (!current) return false;
        auto& callback = current->action.isEnabled;
        return callback ? callback(graph, selection) : true;
    };

    if (registration->action.trigger)
    {
        snapshot.trigger = [id](Graph& graph,
                                QVector<ObjectUuid> const& selection,
                                QWidget* parent) {
            auto current = findRegistration(id);
            if (!current) return;
            auto& callback = current->action.trigger;
            if (callback) callback(graph, selection, parent);
        };
    }

    return snapshot;
}

} // namespace

QPointer<QMenu>
intelli::detail::createGraphSelectionMenu(QWidget* parent)
{
    return QPointer<QMenu>{new QMenu{parent}};
}

void
intelli::registerGraphSelectionAction(QObject& owner, GraphSelectionAction action)
{
    assertApplicationThread("registerGraphSelectionAction");
    Q_ASSERT_X(owner.thread() == QCoreApplication::instance()->thread(),
               "registerGraphSelectionAction",
               "the registration owner must belong to the application thread");

    auto& actions = registeredActions();
    RegisteredActionPtr retired;
    if (!action.id.isEmpty())
    {
        auto existing = std::find_if(actions.begin(), actions.end(), [&action](auto const& entry) {
            return entry->action.id == action.id;
        });
        if (existing != actions.end())
        {
            retired = std::move(*existing);
            actions.erase(existing);
            QObject::disconnect(retired->ownerDestroyed);
        }
    }

    auto registration = std::make_shared<RegisteredAction>();
    registration->registrationId = nextRegistrationId();
    registration->owner = &owner;
    registration->action = std::move(action);

    auto const id = registration->registrationId;
    registration->ownerDestroyed = QObject::connect(
        &owner, &QObject::destroyed, [id]() {
            removeRegistration(id);
        });

    actions.push_back(std::move(registration));
}

QVector<GraphSelectionAction>
intelli::graphSelectionActions()
{
    assertApplicationThread("graphSelectionActions");
    QVector<GraphSelectionAction> snapshots;
    auto registrations = registeredActions();
    for (auto const& registration : registrations)
    {
        if (registration->owner)
        {
            snapshots.push_back(makeSnapshot(registration));
        }
    }
    return snapshots;
}

bool
intelli::detail::addGraphSelectionActionsToMenu(
        QMenu& menu, Graph& graph, QVector<ObjectUuid> selection)
{
    QPointer<QMenu> menuPointer = &menu;
    QPointer<Graph> graphPointer = &graph;

    selection.erase(std::remove_if(selection.begin(), selection.end(), [](auto const& uuid) {
        return uuid.isEmpty();
    }), selection.end());
    std::sort(selection.begin(), selection.end());
    selection.erase(std::unique(selection.begin(), selection.end()), selection.end());

    for (GraphSelectionAction action : graphSelectionActions())
    {
        if (action.text.isEmpty() || !action.trigger)
        {
            continue;
        }

        bool const isVisible = !action.isVisible ||
                               action.isVisible(*graphPointer, selection);
        if (!menuPointer || !graphPointer) return false;
        if (!isVisible)
        {
            continue;
        }

        QAction* menuAction = menuPointer->addAction(action.text);
        if (!action.icon.isNull())
        {
            menuAction->setIcon(action.icon);
        }
        if (action.isEnabled)
        {
            bool const isEnabled = action.isEnabled(*graphPointer, selection);
            if (!menuPointer || !graphPointer) return false;
            menuAction->setEnabled(isEnabled);
        }

        QPointer<QWidget> parent = menuPointer->parentWidget();
        QObject::connect(menuAction, &QAction::triggered, menuPointer,
                         [action = std::move(action),
                          graphPointer,
                          selection,
                          parent]() {
            if (graphPointer && action.trigger)
            {
                action.trigger(*graphPointer, selection, parent);
            }
                         });
    }
    return true;
}
