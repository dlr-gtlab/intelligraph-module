#include <gtest/gtest.h>

#include <intelli/globals.h>

TEST(StrongTypes, init_value)
{
    using namespace intelli;

    StrongType<int, struct Bla_, 42> st;
    EXPECT_EQ(st, 42);
}

TEST(StrongTypes, default_constructed_type_is_invalid)
{
    using namespace intelli;

    PortId id;
    EXPECT_EQ(id, invalid<PortId>());
}

TEST(StrongTypes, object_stores_correct_value)
{
    using namespace intelli;

    PortId id{42};

    EXPECT_EQ(id, 42);
    EXPECT_EQ(id.value(), 42);

    id = PortId{12};
    EXPECT_EQ(id, 12);
}

TEST(StrongTypes, compare_different_types)
{
    using namespace intelli;

    PortId id{42};
    PortIndex idx{42};
    NodeId nodeId{12};

    EXPECT_EQ(id, idx);
    EXPECT_NE(id, nodeId);
}

TEST(StrongTypes, compare_equal)
{
    using namespace intelli;

    PortId id{42};

    EXPECT_EQ(id, id);
}

TEST(StrongTypes, compare_not_equal)
{
    using namespace intelli;

    PortId id1{42}, id2{12};

    EXPECT_NE(id1, id2);
}

TEST(StrongTypes, compare_greater_than)
{
    using namespace intelli;

    PortId id1{42}, id2{12};

    EXPECT_GT(id1, id2);
    EXPECT_GE(id1, id1);
}

TEST(StrongTypes, compare_less_than)
{
    using namespace intelli;

    PortId id1{42}, id2{12};

    EXPECT_LT(id2, id1);
    EXPECT_LE(id2, id2);
}

TEST(StrongTypes, add)
{
    using namespace intelli;

    PortId id1{42}, id2{12};
    id1 += id2;
    EXPECT_EQ(id1, 54);
}

TEST(StrongTypes, subtract)
{
    using namespace intelli;

    PortId id1{42}, id2{12};
    id1 -= id2;
    EXPECT_EQ(id1, 30);
}

TEST(StrongTypes, multiply)
{
    using namespace intelli;

    PortId id1{10}, id2{12};
    id1 *= id2;
    EXPECT_EQ(id1, 120);
}

TEST(StrongTypes, division)
{
    using namespace intelli;

    PortId id1{120}, id2{12};
    id1 /= id2;
    EXPECT_EQ(id1, 10);
}

TEST(StrongTypes, increment)
{
    using namespace intelli;

    PortId id{1};
    id++;
    EXPECT_EQ(id, 2);
    ++id;
    EXPECT_EQ(id, 3);

    EXPECT_EQ(id++, 3);
    EXPECT_EQ(id, 4);
}

TEST(StrongTypes, decrement)
{
    using namespace intelli;

    PortId id{5};
    id--;
    EXPECT_EQ(id, 4);
    --id;
    EXPECT_EQ(id, 3);

    EXPECT_EQ(id--, 3);
    EXPECT_EQ(id, 2);
}

#include "nodes/numbermathnode.h"

TEST(StrongTypes, bla)
{
    intelli::test::NumberMathNode node1, node2;

    auto* copiedObject = &node1;
    auto* originalObject = &node2;

    QObject::connect(originalObject, &intelli::Node::nodeChanged,
                     [](){
                gtDebug() << "HELLO WORLD";
            });

    emit copiedObject->inputDataRecieved(intelli::PortIndex{42});

    const QMetaObject* sourceMetaObject = copiedObject->metaObject();

    // Iterate through the slots and signals of the sourceObject's meta object
    for (int i = 0; i < sourceMetaObject->methodCount(); ++i) {
        QMetaMethod sourceMethod = sourceMetaObject->method(i);

        // Check if the method is a signal (outgoing connection)
        if (sourceMethod.methodType() == QMetaMethod::Signal) {
            // Get the signal's signature
            QByteArray signalSignature = sourceMethod.methodSignature();

            // Find the corresponding signal in the targetObject's meta object
            int signalIndex = originalObject->metaObject()->indexOfSignal(signalSignature);
            if (signalIndex != -1) {
                gtDebug() << signalIndex << "connecting" << signalSignature << sourceMethod.enclosingMetaObject()->className();
                // Connect the signal from sourceObject to the corresponding signal in targetObject
                QObject::connect(copiedObject, sourceMethod, originalObject, originalObject->metaObject()->method(signalIndex));
            }
        }
    }

    emit copiedObject->nodeChanged();
}
