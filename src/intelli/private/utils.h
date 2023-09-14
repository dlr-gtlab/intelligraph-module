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

#include <gt_logstream.h>

#include <gt_intproperty.h>

#include <QtNodes/Definitions>

inline gt::log::Stream&
operator<<(gt::log::Stream& s, QtNodes::ConnectionId const& con)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "NodeConnection["
            << con.outNodeId << ":" << con.outPortIndex << "/"
            << con.inNodeId  << ":" << con.inPortIndex  << "]";
    }
    return s;
}

inline gt::log::Stream&
operator<<(gt::log::Stream& s, std::shared_ptr<intelli::NodeData const> const& data)
{
    // temporary
    if (auto* d = qobject_cast<intelli::DoubleData const*>(data.get()))
    {
        gt::log::StreamStateSaver saver(s);
        return s.nospace() << data->metaObject()->className() << " (" <<d->value() << ")";
    }

    return s << (data ? data->metaObject()->className() : "nullptr");
}

namespace intelli
{

inline constexpr static PortType          convert(QtNodes::PortType type) { return static_cast<PortType>(type); }
inline constexpr static QtNodes::PortType convert(PortType type)          { return static_cast<QtNodes::PortType>(type); }

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
auto ignoreSignal(Sender sender, SignalSender signalSender,
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
