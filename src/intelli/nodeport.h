/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEPORT_H
#define GT_INTELLI_NODEPORT_H

#include <intelli/globals.h>

#include <gt_logging.h>

namespace intelli
{

/// Port info struct
class NodePort
{
public:

    // cppcheck-suppress noExplicitConstructor
    NodePort(QString _typeId) : NodePort(std::move(_typeId), {}) {}

    NodePort(QString _typeId,
             QString _caption,
             bool _captionVisible = true,
             bool _optional = true) :
        typeId(std::move(_typeId)),
        caption(std::move(_caption) ),
        captionVisible(_captionVisible),
        optional(_optional)
    {}

    NodePort(NodePort const& other) = default;
    NodePort(NodePort&& other) = default;
    NodePort& operator=(NodePort const& other) = delete;
    NodePort& operator=(NodePort&& other) = default;
    ~NodePort() = default;

    /// creates a NodePort struct with a custom port id
    template<typename ...T>
    static NodePort customId(PortId newPortId, T&&... args)
    {
        NodePort pd(std::forward<T>(args)...);
        pd.m_id = newPortId;
        return pd;
    }

    /// Performs a copy assignment using `other` but keeps old id
    void assign(NodePort other)
    {
        NodePort tmp(std::move(other));
        tmp.m_id = m_id;
        swap(tmp);
    }

    /// creates a copy of this object but resets the id parameter
    NodePort copy() const
    {
        NodePort pd(*this);
        pd.m_id = invalid<PortId>();
        return pd;
    }

    /// swap
    void swap(NodePort& other) noexcept
    {
        using std::swap;
        swap(typeId, other.typeId);
        swap(caption, other.caption);
        swap(captionVisible, other.captionVisible);
        swap(visible, other.visible);
        swap(optional, other.optional);
        swap(m_isConnected, other.m_isConnected);
        swap(m_id, other.m_id);
    }

    // setters to allow function chaining
    NodePort& setCaption(QString v) { caption = std::move(v); return *this; }
    NodePort& setToolTip(QString v) { toolTip = std::move(v); return *this; }
    NodePort& setCaptionVisible(bool v) { captionVisible = v; return *this; }
    NodePort& setVisible(bool v) { visible = v; return *this; }
    NodePort& setOptional(bool v) { optional = v; return *this; }

    /// type id for port data (classname)
    TypeId typeId;
    /// custom port caption (optional)
    QString caption;
    /// custom tooltip
    QString toolTip;
    /// whether port caption should be visible
    bool captionVisible = true;
    /// whether the port is visible at all
    bool visible = true;
    /// whether the port is required for the node evaluation
    bool optional = true;

    /**
         * @brief Returns the port id
         * @return port id
         */
    inline PortId id() const { return m_id; }

    /**
         * @brief Whether port is connected
         * @return Whether port is connected
         */
    inline bool isConnected() const { return m_isConnected; }

private:
    /// whether port is connected
    bool m_isConnected{false};
    /// read only PortId
    PortId m_id{};

    friend class Node;
};

using PortInfo = NodePort;

} // namespace intelli

namespace gt
{
namespace log
{

inline Stream&
operator<<(Stream& s, intelli::NodePort const& p)
{
    {
        StreamStateSaver saver(s);
        s.nospace()
            << "Port[" << p.typeId << "/" << p.id() << "]";
    }
    return s.doLogSpace();
}

} // namespace log

} // namespace gt

#endif // GT_INTELLI_NODEPORT_H
