/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_UTILS_H
#define GT_INTELLI_UTILS_H

#include <intelli/globals.h>
#include <intelli/data/double.h>
#include <chrono>

#include <gt_logstream.h>
#include <gt_platform.h>

#include <gt_intproperty.h>

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

class Profiler
{
public:
    Profiler(const char* text = "") :
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
    const char* m_text = {};
    std::chrono::high_resolution_clock::time_point m_start;
};

template <typename Sender, typename SignalSender,
         typename Reciever, typename SignalReciever>
struct IgnoreSignal
{
    IgnoreSignal(Sender sender_, SignalSender signalSender_,
                 Reciever reciever_, SignalReciever signalReciever_) :
        sender(sender_), signalSender(signalSender_), reciever(reciever_), signalReciever(signalReciever_)
    {
        QObject::disconnect(sender, signalSender, reciever, signalReciever);
    }

    ~IgnoreSignal()
    {
        QObject::connect(sender, signalSender, reciever, signalReciever, Qt::UniqueConnection);
    }

    Sender sender;
    SignalSender signalSender;
    Reciever reciever;
    SignalReciever signalReciever;
};

template <typename Sender, typename SignalSender,
         typename Reciever, typename SignalReciever>
GT_NO_DISCARD auto
ignoreSignal(Sender sender, SignalSender signalSender,
             Reciever reciever, SignalReciever signalReciever)
{
    return IgnoreSignal<Sender, SignalSender, Reciever, SignalReciever>{
        sender, signalSender, reciever, signalReciever
    };
}

template <typename T>
inline QString toString(T const& t)
{
    gt::log::Stream s;
    s.nospace() << t;
    return QString::fromStdString(s.str());
}

} // namespace intelli

#endif // GT_INTELLI_UTILS_H
