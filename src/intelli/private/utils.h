/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_UTILS_H
#define GT_INTELLI_UTILS_H

#include <intelli/globals.h>
#include <intelli/data/double.h>
#include <intelli/graph.h>

#include <gt_state.h>
#include <gt_statehandler.h>
#include <gt_utilities.h>
#include <gt_qtutilities.h>
#include <gt_regexp.h>

#include <gt_logstream.h>

#include <QTimer>
#include <QThread>

#include <algorithm>

#define GT_INTELLI_PROFILE() \
intelli::Profiler profiler__{__FUNCTION__}; (void)profiler__;
#define GT_INTELLI_PROFILE_C(X) \
intelli::Profiler profiler__{X}; (void)profiler__;

inline gt::log::Stream&
operator<<(gt::log::Stream& s, std::shared_ptr<intelli::NodeData const> const& data)
{
    // TODO: remove
    if (auto* d = qobject_cast<intelli::DoubleData const*>(data.get()))
    {
        gt::log::StreamStateSaver saver(s);
        return s.nospace() << data->metaObject()->className() << " (" <<d->value() << ")";
    }

    return s << (data ? data->metaObject()->className() : "nullptr");
}

namespace intelli
{

/// using std::is_const
template<typename T>
using is_const = std::is_const<std::remove_reference_t<T>>;

/// apply const to `T` if `IsConst` is true
template<bool IsConst, typename T>
struct apply_const;
template<typename T>
struct apply_const<true, T> { using type = std::add_const_t<T>; };
template<typename T>
struct apply_const<false, T> { using type = std::remove_const_t<T>; };

/// wrapper to apply const to `T` if `IsConst` is true
template<bool IsConst, typename T>
using const_t = typename apply_const<IsConst, T>::type;
/// wrapper to apply constness of `U` to `T`
template<typename U, typename T>
using apply_constness_t = typename apply_const<is_const<U>::value, T>::type;

/// Retrieves iterator type for `QHash` depending on `IsConst`
template<bool IsConst, typename T>
struct get_iterator;
template<typename T>
struct get_iterator<true, T> { using type = typename T::const_iterator; };
template<typename T>
struct get_iterator<false, T> { using type = typename T::iterator; };
template<bool IsConst, typename T>
using get_iterator_t = typename get_iterator<IsConst, T>::type;

/// Profiler helper class
class Profiler
{
public:
    Profiler(char const* text = "") :
        m_text(text),
        m_start(std::chrono::high_resolution_clock::now())
    { }
    ~Profiler()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();

        if (duration > 0) gtTrace() << "[PROFILER]" << m_text << "- took" << duration << "us";
    }

private:
    char const* m_text = {};
    std::chrono::high_resolution_clock::time_point m_start;
};

/// Converts `t` into a `QString` using `gt::log::Stream`
template <typename T>
inline QString toString(T const& t)
{
    gt::log::Stream s;
    s.nospace() << t;
    return QString::fromStdString(s.str());
}

namespace utils
{

/// Helper function that searches for `t` in List and returns the iterator
template<typename List, typename T>
inline auto
contains(List const& list, T const& t)
{
    return std::find(list.begin(), list.end(), t) != list.end();
}

/// Helper function that searches for `t` in List and erases it if `t` was found
template<typename List, typename T>
inline bool
erase(List& list, T const& t)
{
    auto iter = std::find(list.begin(), list.end(), t);
    if (iter != list.end())
    {
        list.erase(iter);
        return true;
    }
    return false;
}

/// Helper function that returns the path of the node as a formated string for
/// logging
inline QString
logId(Node const& node)
{
    return gt::quoted(relativeNodePath(node), "[", "]");
}

/// Helper function that returns the class name of the template parameter as a
/// formated string for logging
template<typename T>
inline QString logId()
{
    static QString str = QString{T::staticMetaObject.className()}.remove("intelli::");
    return gt::quoted(str, "[", "]");
}

/// Helper function to deduce class name from argument and return a formated
/// string for logging
template<typename T,
         typename U = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>,
         std::enable_if_t<!std::is_base_of<Node, U>::value, bool> = true>
inline QString logId(T const&) { return logId<U>(); }

/// helper struct to make state creation more explicit and ledgible
template <typename GetValue>
struct SetupStateHelper
{
    GtState* state = nullptr;
    GetValue getValue;

    /**
     * @brief Registers a slot that is called when the state's value changes
     * @param reciever Reciever object
     * @param slot Slot object to call when the state changes its value
     * @return Reference for operator chaining
     */
    template<typename Reciever,
             typename Slot>
    SetupStateHelper& onStateChange(Reciever* reciever, Slot slot)
    {
        QObject::connect(state, qOverload<QVariant const&>(&GtState::valueChanged),
                         reciever, slot);
        return *this;
    }

    /**
     * @brief Registers a signal that triggers the update of the state
     * @param sender Sender object
     * @param signal Signal to trigger the update of the state
     * @return Reference for operator chaining
     */
    template<typename Signal,
             typename Sender>
    SetupStateHelper& onValueChange(Sender* sender, Signal signal)
    {
        QObject::connect(sender, signal, state, [s = state, value = getValue](){
            constexpr bool undoCommand = false;
            s->setValue(value(), undoCommand);
        });
        return *this;
    }

    /**
     * @brief Finalizes the state creation by triggering the `onStateChanged`
     * slot in the next event cycle.
     */
    GtState* finalize()
    {
        QTimer::singleShot(0, [state = this->state](){
            emit state->valueChanged(state->getValue());
        });
        return state;
    }
};

/**
 * @brief Helper function to create a state and update it accordingly when
   a signal is triggered.
 * @param guardian Guardian object that keeps the state alive
 * @param graph Graph to register state for
 * @param stateId State text
 * @param getValue Getter for the value
 * @return State helper used to further setup the state
 */
template<typename SourceObject, typename GetValue>
inline SetupStateHelper<GetValue>
setupState(GtObject& guardian,
           Graph const& graph,
           QString const& stateId,
           GetValue getValue)
{
    auto* root = graph.rootGraph();
    assert(root);

    /// grid change state
    auto* state = gtStateHandler->initializeState(
        // group id
        root->uuid() + QChar('(') + GT_CLASSNAME(Graph) + QChar(')'),
        // state id
        stateId,
        // entry for this graph
        QString{GT_CLASSNAME(SourceObject)} + QChar(';') + stateId.toLower().replace(' ', '_'),
        // default value
        getValue(),
        // guardian object
        &guardian
    );

    return SetupStateHelper<GetValue>{state, std::move(getValue)};
}


inline void setObjectName(GtObject& obj, QString const& name)
{
    obj.setObjectName(name);
}

inline void setObjectName(Node& obj, QString const& name)
{
    obj.setCaption(name);
}



template <typename T>
inline void restrictRegExpWithSiblingsNames(GtObject& obj,
                                            QRegExp& defaultRegExp)
{
    GtObject* parent = obj.parentObject();

    if (!parent) return;

    QList<T*> siblings = parent->findDirectChildren<T*>();

    if (siblings.isEmpty()) return;

    QStringList names;

    for (auto* s : qAsConst(siblings))
    {
        names.append(s->objectName());
    }

    names.removeAll(obj.objectName());

    QString pattern = std::accumulate(
        std::begin(names), std::end(names), QString{'^'},
        [](QString const& a, QString const& name) {
            return a + "(?!" + name + "$)";
        });

    pattern += defaultRegExp.pattern();

    defaultRegExp = QRegExp(pattern);
}

#ifdef GAMEPAD_USAGE

// Linken der XInput-Bibliothek
#pragma comment(lib, "Xinput.lib")

// GamepadThread: Pollt den Controller und emittiert bei Tastendruck ein Signal.
class GamepadThread : public QThread {
    Q_OBJECT
public:
    explicit GamepadThread(QObject* parent = nullptr)
        : QThread(parent) {}

signals:
    // Signal, das emittiert wird, wenn der A-Button gedrückt wurde.
    void buttonPressed(const QString &buttonName);

protected:
    void run() override;
};
#endif

} // namespace utils

} // namespace intelli

#endif // GT_INTELLI_UTILS_H
