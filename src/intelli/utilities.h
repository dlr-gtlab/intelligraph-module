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

#include <intelli/exports.h>
#include <intelli/globals.h>
#include <intelli/view.h>

#include <gt_finally.h>

namespace intelli
{

class Graph;

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
 * @brief Connects a signal to a signal/slot using QObject::connect and returns
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
 * @brief Copies the object given their uuid from the source graph to the
 * target graph. The original objects are not deleted. The selected objects
 * may include nodes and comments. All connections inbetween nodes are copied
 * even if they are not selected. Copied objects are assigned a new UUID.
 *
 * @param source Source graph. Selected objects are only copied and remain in
 * the source graph.
 * @param selection Vector-like list of uuids to copy
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool copyObjectsToGraph(Graph const& source, View<ObjectUuid> selection, Graph& target);

/**
 * @brief Overload that copies all objects from the source graph to the target
 * graph.
 *
 * @param source Source graph. Objects are only copied and remain in
 * the source graph.
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool copyObjectsToGraph(Graph const& source, Graph& target);

GT_INTELLI_EXPORT
bool moveObjectsToGraph(Graph& source, View<ObjectUuid> selection, Graph& target);

/**
 * @brief Moves all objects from the source graph to the target graph.
 * @param source Source graph. Objects are moved and thus do not remain in
 * the source graph.
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool moveObjectsToGraph(Graph& source, Graph& target);

} // namespace utils

} // namespace intelli

#endif // GT_INTELLI_UTILITIES_H
