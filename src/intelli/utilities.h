/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_UTILITIES_H
#define GT_INTELLI_UTILITIES_H

#include <intelli/globals.h>

#include <gt_finally.h>

namespace intelli
{

namespace utils
{

/**
 * @brief Quantizes `point` so that it is a multiple of `stepSize`.
 * Example:
 *     quantize(QPointF(42.4,9.75), 5) -> QPoint(40, 10)
 * @param point Point to quantize
 * @param stepSize Step size to use for quantization. Must not be zero.
 * @return quantized point
 */
GT_NO_DISCARD
inline QPoint quantize(QPointF point, int stepSize)
{
    assert(stepSize > 0);
    auto divX = std::div(point.x(), stepSize);
    auto divY = std::div(point.y(), stepSize);
    double x = divX.rem;
    double y = divY.rem;

    double stepHalf = 0.5 * stepSize;
    divX.quot += x > stepHalf ? 1 : x < -stepHalf ? -1 : 0;
    divY.quot += y > stepHalf ? 1 : y < -stepHalf ? -1 : 0;

    return QPoint{divX.quot * stepSize, divY.quot * stepSize};
};

/**
 * @brief Maps a value of an input range onto an output range
 * @param value Value to map between input and output range
 * @param inputRange Input range Input range
 * @param outputRange Output range
 * @return value mapped to output range
 */
template <typename U, typename T = U>
GT_NO_DISCARD
constexpr
inline U map(T value, std::pair<T, T> inputRange, std::pair<U, U> outputRange)
{
    constexpr size_t start = 0, end = 1;

    double slope = double(std::get<end>(outputRange) - std::get<start>(outputRange)) /
                   double(std::get<end>(inputRange)  - std::get<start>(inputRange));

    return static_cast<U>(std::get<start>(outputRange) +
                          slope * (value - std::get<start>(inputRange)));
}

/**
 * @brief Helper struct that allows the use of for-range based loops and similar
 * constructs
 */
template <typename T>
struct iterable
{
    T b, e;

    T begin() const { return b; }
    T end() const { return e; }

    bool empty() const { return begin() == end(); }
    size_t size() const { return std::distance(begin(), end()); }
};

/**
 * @brief Helper method that instantiates an iterable object
 */
template <typename Iterator>
inline iterable<Iterator> makeIterable(Iterator begin, Iterator end)
{
    return {std::move(begin), std::move(end)};
}

/**
 * @brief Helper method that instantiates an iterable object based on `t`'s
 * begin and end operator.
 */
template <typename T>
inline auto makeIterable(T& t)
{
    return makeIterable(t.begin(), t.end());
}

/**
 * @brief Helper method that instantiates an iterable object based on `t`'s
 * reverse begin and end operator.
 */
template <typename T>
inline auto makeReverseIterable(T& t)
{
    return makeIterable(t.rbegin(), t.rend());
}

template <typename Sender, typename SignalSender,
         typename Reciever, typename SignalReciever>
struct IgnoreSignal
{
    IgnoreSignal(Sender sender_, SignalSender signalSender_,
                 Reciever reciever_, SignalReciever signalReciever_) :
        sender(sender_), signalSender(signalSender_),
        reciever(reciever_), signalReciever(signalReciever_)
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

/**
 * @brief Ignores a signal-sginal/signal-slot connection between two objects
 * for the lifetime of the returned helper object.
 *
 * Note: Only works for connections with function pointers or
 * meta object connections. Best to use for signals with `Qt::UniqueConnection`.
 *
 * @param sender Sender
 * @param signalSender Signal of sender
 * @param reciever Reciever
 * @param signalReciever Signal/Slot of reciever
 * @returns Scoped object. Connection is reestablished once the object is
 * deleted.
 */
template<typename Sender, typename SignalSender,
         typename Reciever, typename SignalReciever>
GT_NO_DISCARD
inline auto ignoreSignal(Sender sender, SignalSender signalSender,
                         Reciever reciever, SignalReciever signalReciever)
{
    return IgnoreSignal<Sender, SignalSender, Reciever, SignalReciever>{
        sender, signalSender, reciever, signalReciever
    };
}

/**
 * @brief Connects a signal/slot to a signal using QObject::connect and returns
 * a scoped object, that deletes the established connection once the object is
 * deleted. Useful for temporary signals.
 * @param sender Sender
 * @param signalSender Signal of sender
 * @param reciever Reciever
 * @param signalReciever Signal/Slot of reciever
 * @returns Scoped object. Connection is destroyed once the object is deleted.
 */
template<typename Sender, typename SignalSender,
         typename Reciever, typename SignalReciever>
GT_NO_DISCARD
inline auto connectScoped(Sender sender, SignalSender signalSender,
                          Reciever reciever, SignalReciever signalReciever)
{
    auto connection = QObject::connect(sender, signalSender,
                                       reciever, signalReciever);

    return gt::finally([sender, connection](){
        sender->disconnect(connection);
    });
}

/**
 * @brief Connects a slot-functor to a s signal using QObject::connect.
 * The slot is triggered only once. The slot argument must be a functor,
 * function pointers are not supported yet.
 * @param sender Sender
 * @param signalSender Signal of sender
 * @param reciever Reciever (scope)
 * @param signalReciever Slot functor
 */
template<typename Sender, typename SignalSender,
         typename Reciever, typename SignalReciever>
inline void connectOnce(Sender sender, SignalSender signalSender,
                        Reciever reciever, SignalReciever signalReciever,
                        Qt::ConnectionType type = Qt::DirectConnection)
{
    auto* ctx = new QObject{reciever};
    QObject::connect(sender, signalSender, ctx, [=]() {
        signalReciever();
        ctx->deleteLater();
    }, type);
}

} // namespace utils

} // namespace intelli

#endif // GT_INTELLI_UTILITIES_H
