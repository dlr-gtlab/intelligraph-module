/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/graphics/connectionobject.h>
#include <intelli/gui/graphics/commentobject.h>
#include <intelli/gui/graphics/lineobject.h>

#if 0
#include <intelli/gui/nodeui.h>
#include <intelli/gui/commentobject.h>
#include <intelli/node.h>

#include "node/test_node.h"
#endif

TEST(GUI, type_ids)
{
    using namespace intelli;

    auto types = {
        (unsigned)GraphicsObject::Type,
        (unsigned)InteractableGraphicsObject::Type,
        (unsigned)NodeGraphicsObject::Type,
        (unsigned)ConnectionGraphicsObject::Type,
        (unsigned)CommentGraphicsObject::Type,
        (unsigned)LineGraphicsObject::Type,
    };

    for (auto type : types)
    {
        // each type should be unique
        size_t count = std::count(types.begin(), types.end(), type);
        EXPECT_EQ(count, 1);
    }
}

#if 0
TEST(GUI, graphcis_cast)
{
    using namespace intelli;

    GraphicsObject* basePtr = nullptr;
    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<InteractableGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<GraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<CommentGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<LineGraphicsObject*>(basePtr));

    TestNode node;
    NodeUI nodeUi;
    GraphSceneData sceneData;
    NodeGraphicsObject nodeObj{sceneData, node, nodeUi};
    basePtr = &nodeObj;

    EXPECT_TRUE(graphics_cast<NodeGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<InteractableGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<GraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<CommentGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<LineGraphicsObject*>(basePtr));

    ConnectionGraphicsObject connection{invalid<ConnectionId>()};
    basePtr = &connection;

    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<InteractableGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<GraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<CommentGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<LineGraphicsObject*>(basePtr));

    CommentObject co{};
    CommentGraphicsObject comment{co, sceneData};
    basePtr = &comment;

    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<InteractableGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<GraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<CommentGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<LineGraphicsObject*>(basePtr));

    LineGraphicsObject line{comment};
    basePtr = &line;

    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<InteractableGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<GraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<ConnectionGraphicsObject*>(basePtr));
    EXPECT_TRUE(graphics_cast<CommentGraphicsObject*>(basePtr));
    EXPECT_FALSE(graphics_cast<LineGraphicsObject*>(basePtr));
}
#endif
