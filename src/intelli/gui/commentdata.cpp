/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/commentdata.h>

#include <gt_stringproperty.h>
#include <gt_intproperty.h>
#include <gt_doubleproperty.h>
#include <gt_boolproperty.h>
#include <gt_structproperty.h>
#include <gt_stringproperty.h>
#include <gt_propertystructcontainer.h>

#include <gt_coreapplication.h>

#include <QSize>

using namespace intelli;

struct CommentData::Impl
{
    Impl() { }

    /// text of comment
    GtStringProperty text{
        "text", QObject::tr("Text"), QObject::tr("Comment Text")
    };
    /// x position of comment
    GtDoubleProperty posX{
        "posX", QObject::tr("x-Pos"), QObject::tr("x-Position")
    };
    /// y position of comment
    GtDoubleProperty posY{
        "posY", QObject::tr("y-Pos"), QObject::tr("y-Position")
    };

    /// width of comment widget
    GtIntProperty sizeWidth{
        "sizeWidth",
        QObject::tr("Size Width"),
        QObject::tr("Size Width"), -1
    };
    /// height of comment widget
    GtIntProperty sizeHeight{
        "sizeHeight",
        QObject::tr("Size Height"),
        QObject::tr("Size Height"), -1
    };

    /// whether the comment is collapsed
    GtBoolProperty collapsed{
        "collapsed", QObject::tr("Collapsed"), QObject::tr("Collapsed"), false
    };

    /// whether the comment is centered
    GtBoolProperty textCentered{
        "textCentered", QObject::tr("Text Centered"),
        QObject::tr("Whether to center the text of the comment"),
        false
    };

    /// whether the comment's border should be displayed
    GtBoolProperty showBorder{
        "showBorder", QObject::tr("Show Broder"),
        QObject::tr("Whether to show the comment's border"),
        true
    };

    GtPropertyStructContainer connections{
        "connections", tr("Connected Objects")
    };
    /// TODO: remove me once core issue #1366 is merged
    QVector<NodeId> connectionsData;

    /// text color
    GtStringProperty textColor{
        "textColor", QObject::tr("Text Color"),
        QObject::tr("Color of the text")
    };

    /// text background color
    GtStringProperty bgColor{
        "bgColor", QObject::tr("Background Color"),
        QObject::tr("Color of the background")
    };
};

char const* S_CONNECTION_DATA_TYPE_ID = "ConnectionData";

#define ASSERT_EQ_SIZE() assert(pimpl->connections.size() == (size_t)pimpl->connectionsData.size())

CommentData::CommentData(GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<Impl>())
{
    setFlag(UserDeletable);

    registerProperty(pimpl->posX);
    registerProperty(pimpl->posY);
    registerProperty(pimpl->sizeWidth);
    registerProperty(pimpl->sizeHeight);
    registerProperty(pimpl->collapsed);
    registerProperty(pimpl->text);

    QString displayCat = tr("Display");
    registerProperty(pimpl->textCentered, displayCat);
    registerProperty(pimpl->showBorder, displayCat);
    registerProperty(pimpl->textColor, displayCat);
    registerProperty(pimpl->bgColor, displayCat);

    // presence of struct indicates that the node is collapsed
    GtPropertyStructDefinition structType{S_CONNECTION_DATA_TYPE_ID};
    pimpl->connections.registerAllowedType(structType);

    registerPropertyStructContainer(pimpl->connections);

    pimpl->posX.setReadOnly(true);
    pimpl->posY.setReadOnly(true);
    pimpl->sizeWidth.setReadOnly(true);
    pimpl->sizeHeight.setReadOnly(true);
    pimpl->collapsed.setReadOnly(true);
    pimpl->text.setReadOnly(true);

#ifndef GT_INTELLI_DEBUG_NODE_PROPERTIES
    bool hide = !gtApp || !gtApp->devMode();
    pimpl->posX.hide(true);
    pimpl->posY.hide(true);
    pimpl->sizeWidth.hide(true);
    pimpl->sizeHeight.hide(true);
    pimpl->collapsed.hide(true);
    pimpl->text.hide(true);
#endif
    connect(&pimpl->collapsed, &GtAbstractProperty::changed, this, [this](){
        emit commentCollapsedChanged(pimpl->collapsed.get());
    });

    // must subscribe to both properties incase only one changes
    connect(&pimpl->posX, &GtAbstractProperty::changed,
            this, &CommentData::commentPositionChanged);

    connect(&pimpl->posY, &GtAbstractProperty::changed,
            this, &CommentData::commentPositionChanged);

    // must subscribe to both properties incase only one changes
    connect(&pimpl->sizeWidth, &GtAbstractProperty::changed,
            this, &CommentData::commentSizeChanged);
    connect(&pimpl->sizeHeight, &GtAbstractProperty::changed,
            this, &CommentData::commentSizeChanged);

    connect(&pimpl->text, &GtAbstractProperty::changed,
            this, &CommentData::commentChanged);

    connect(&pimpl->textCentered, &GtAbstractProperty::changed,
            this, &CommentData::commentAlignmentChanged);

    connect(&pimpl->showBorder, &GtAbstractProperty::changed, this, [this](){
        emit commentBorderVisibilityChanged(showBorder());
    });

    connect(&pimpl->bgColor, &GtAbstractProperty::changed,
            this, &CommentData::commentColorChanged);

    connect(&pimpl->textColor, &GtAbstractProperty::changed,
            this, &CommentData::commentColorChanged);

    setObjectName(QStringLiteral("comment_%1")
                      .arg(uuid().remove("{").remove("-").mid(0, 8)));

    connect(&pimpl->connections, &GtPropertyStructContainer::entryAdded,
            this, [this](int idx){
        NodeId nodeId = nodeConnectionAt(idx);
        if (!nodeId.isValid())
        {
            pimpl->connectionsData.insert(idx, invalid<NodeId>());
            ASSERT_EQ_SIZE();
            return;
        }
        pimpl->connectionsData.insert(idx, nodeId);
        emit nodeConnectionAppended(nodeId);
        ASSERT_EQ_SIZE();
    }, Qt::DirectConnection);

    connect(&pimpl->connections, &GtPropertyStructContainer::entryRemoved,
            this, [this](int idx){
        NodeId nodeId = pimpl->connectionsData.at(idx);
        pimpl->connectionsData.removeAt(idx);
        emit nodeConnectionRemoved(nodeId);
        ASSERT_EQ_SIZE();
    }, Qt::DirectConnection);

}

CommentData::~CommentData()
{
    emit aboutToBeDeleted();
}

void
CommentData::setText(QString text)
{
    pimpl->text = std::move(text);
}

QString const&
CommentData::text() const
{
    return pimpl->text.get();
}

void
CommentData::setPos(Position pos)
{
    if (this->pos() != pos)
    {
        pimpl->posX = pos.x();
        pimpl->posY = pos.y();
        changed();
    }
}

Position
CommentData::pos() const
{
    return { pimpl->posX, pimpl->posY };
}

void
CommentData::setSize(QSize size)
{
    if (this->size() != size)
    {
        pimpl->sizeWidth = size.width();
        pimpl->sizeHeight = size.height();
        changed();
    }
}

QSize
CommentData::size() const
{
    return { pimpl->sizeWidth, pimpl->sizeHeight };
}

void
CommentData::setCollapsed(bool collapsed)
{
    if (this->isCollapsed() != collapsed)
    {
        pimpl->collapsed = collapsed;
        changed();
    }
}

bool
CommentData::isCollapsed() const
{
    return pimpl->collapsed;
}

void
CommentData::setTextCentered(bool centered)
{
    if (this->isTextCentered() != centered)
    {
        pimpl->textCentered = centered;
        changed();
    }
}

bool
CommentData::isTextCentered() const
{
    return pimpl->textCentered;
}

void
CommentData::setShowBorder(bool enabled)
{
    if (this->showBorder() != enabled)
    {
        pimpl->showBorder = enabled;
        changed();
    }
}

bool
CommentData::showBorder() const
{
    return pimpl->showBorder;
}

void
CommentData::setTextColor(QString textColor)
{
    pimpl->textColor = std::move(textColor);
}

QString const&
CommentData::textColor() const
{
    return pimpl->textColor.get();
}

void
CommentData::setBackgroundColor(QString textColor)
{
    pimpl->bgColor = std::move(textColor);
}

QString const&
CommentData::backgroundColor() const
{
    return pimpl->bgColor.get();
}

void
CommentData::appendNodeConnection(NodeId targetNodeId)
{
    if (!targetNodeId.isValid() || isNodeConnected(targetNodeId)) return;

    pimpl->connections.newEntry(S_CONNECTION_DATA_TYPE_ID, QString::number(targetNodeId));
    ASSERT_EQ_SIZE();
}

bool
CommentData::removeNodeConnection(NodeId targetNodeId)
{
    auto iter = std::find_if(pimpl->connections.begin(),
                             pimpl->connections.end(),
                             [targetNodeId](GtPropertyStructInstance const& e){
         bool ok = true;
         return e.ident().toUInt(&ok) == targetNodeId && ok;
    });
    if (iter == pimpl->connections.end()) return false;

    pimpl->connections.removeEntry(iter);
    ASSERT_EQ_SIZE();

    return true;
}

bool
CommentData::isNodeConnected(NodeId targetNodeId)
{
    ASSERT_EQ_SIZE();
    return std::find_if(pimpl->connections.begin(),
                        pimpl->connections.end(),
                        [targetNodeId](GtPropertyStructInstance const& e){
        return e.ident().toUInt() == targetNodeId;
    }) != pimpl->connections.end();
}

size_t
CommentData::nNodeConnections() const
{
    return pimpl->connections.size();
}

NodeId
CommentData::nodeConnectionAt(size_t idx) const
{
    assert(idx < pimpl->connections.size() && "idx out of bounds!");

    bool ok = true;
    NodeId nodeId = NodeId::fromValue(pimpl->connections.at(idx).ident().toUInt(&ok));
    return ok ? nodeId : invalid<NodeId>();
}

void
CommentData::onObjectDataMerged()
{
    // remove all invalid connections (i.e. NodeId == invalid<NodeId>())
    auto iter = std::find(pimpl->connectionsData.begin(),
                          pimpl->connectionsData.end(),
                          invalid<NodeId>());
    if (iter == pimpl->connectionsData.end()) return;

    size_t idx = std::distance(pimpl->connectionsData.begin(), iter);
    pimpl->connections.removeEntry(std::next(pimpl->connections.begin(), idx));

    ASSERT_EQ_SIZE();
    return CommentData::onObjectDataMerged(); // check once more
}
