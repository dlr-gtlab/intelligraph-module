/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "gtest/gtest.h"
#include "intelli/span.h"
#include "intelli/view.h"

#include <QVector>
#include <vector>
#include <array>


TEST(Span, default_constructed)
{
    intelli::Span<int> span{};

    EXPECT_TRUE(span.null());
    EXPECT_TRUE(span.empty());
    EXPECT_EQ(span.size(), 0);
    EXPECT_EQ(span.data(), nullptr);
    EXPECT_EQ(span.begin(), span.end());
}

TEST(Span, construction)
{
    QVector<int> qvector{1, 2, 3, 4, 5};
    std::vector<int> stdvector{1, 2, 3, 4, 5};
    std::array<int, 5> stdarray{1, 2, 3, 4, 5};
    int carray[5] = {1, 2, 3, 4, 5};

    intelli::Span<int> span1{qvector};
    intelli::Span<int> span2{stdvector};
    intelli::Span<int> span3{stdarray};
    intelli::Span<int> span4{carray};

    for (const intelli::Span<int>* span: {&span1, &span2, &span3, &span4})
    {
        EXPECT_EQ(span->size(), 5);
        EXPECT_FALSE(span->null());
        EXPECT_FALSE(span->empty());
        EXPECT_EQ((*span)[0], 1);
        EXPECT_EQ(span->front(), 1);
        EXPECT_EQ(span->back(), 5);
    }

    EXPECT_EQ(span1.data(), qvector.data());
    EXPECT_EQ(span2.data(), stdvector.data());
    EXPECT_EQ(span3.data(), stdarray.data());
    EXPECT_EQ(span4.data(), carray);
}

TEST(Span, construction_from_raw_data)
{
    std::vector<int> v{1, 2, 3, 4, 5};

    intelli::Span<int> span{v.data() + 1, 3};

    EXPECT_EQ(span.size(), 3);
    EXPECT_FALSE(span.null());
    EXPECT_FALSE(span.empty());
    EXPECT_EQ(std::vector<int>(span.begin(), span.end()),
              std::vector<int>({2, 3, 4}));
}

TEST(Span, construction_rvalue)
{
    static_assert(!std::is_constructible<intelli::Span<int>, std::vector<int>&&>::value,
                  "Span should not allow construction from rvalues");
    static_assert(std::is_constructible<intelli::View<int>, std::vector<int>&&>::value,
                  "View should allow construction from rvalues");

    auto verify_rvalue = [](intelli::View<int> view) -> void {
        EXPECT_EQ(view.front(), 7);
        EXPECT_EQ(view.back(), 8);
    };

    // temporary containers are allowed for const spans
    verify_rvalue(std::vector<int>{7, 8});
}

TEST(Span, construction_initializer_ist)
{
    static_assert(!std::is_constructible<intelli::Span<int>, std::initializer_list<int>>::value,
                  "Span should not allow construction from initializer lists");
    static_assert(std::is_constructible<intelli::View<int>, std::initializer_list<int>>::value,
                  "View should allow construction from initializer lists");

    intelli::View<int> span{1, 2, 3, 4};

    EXPECT_EQ(span.size(), 4);
    EXPECT_EQ(span[2], 3);
    EXPECT_EQ(span.back(), 4);
}

TEST(Span, modify)
{
    std::vector<int> v{1, 2, 3};
    intelli::Span<int> span{v};

    span[0] = 42;
    span.front() = 43;
    span.back() = 44;

    EXPECT_EQ(v[0], 43);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 44);
}

TEST(Span, iterate)
{
    std::vector<int> v{1, 2, 3, 4, 5};
    const intelli::Span<int> span{v};

    int sum = 0;
    for (auto it = span.cbegin(); it != span.cend(); ++it)
    {
        sum += *it;
    }
    EXPECT_EQ(sum, 15);

    std::vector<int> copy;
    for (const auto& e: span)
    {
        copy.push_back(e);
    }
    EXPECT_EQ(copy, v);
}

TEST(Span, copy)
{
    std::vector<int> v{1, 2, 3};

    intelli::Span<int> span{v};
    intelli::Span<int> span_copy{span};
    intelli::Span<int> spawn_assigned = span;

    intelli::View<int> view{v};
    intelli::View<int> view_copy{view};
    intelli::View<int> view_assigned = view_copy;

    EXPECT_EQ(span_copy.data(), span.data());
    EXPECT_EQ(spawn_assigned.data(), span.data());

    EXPECT_EQ(view_copy.data(), view.data());
    EXPECT_EQ(view_assigned.data(), view.data());

    spawn_assigned[0] = 42;
    EXPECT_EQ(v[0], 42);
    EXPECT_EQ(span[0], 42);
    EXPECT_EQ(view[0], 42);
}

TEST(Span, assign)
{
    std::vector<int> v{1, 2, 3};
    std::vector<int> other{4, 5, 6};

    intelli::Span<int> span{v};
    intelli::Span<int> spawn_other{other};

    intelli::View<int> view{v};
    intelli::View<int> view_other{other};

    span = spawn_other;

    span[0] = 42;
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(other[0], 42);
    EXPECT_EQ(spawn_other[0], 42);
    EXPECT_EQ(view_other[0], 42);
}

TEST(Span, null_container)
{
    std::vector<int> v;
    intelli::Span<int> span{v};
    intelli::View<int> view{v};

    EXPECT_TRUE(span.null());
    EXPECT_TRUE(span.empty());
    EXPECT_EQ(span.size(), 0);
    EXPECT_EQ(span.begin(), span.end());

    EXPECT_TRUE(view.null());
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0);
    EXPECT_EQ(view.begin(), view.end());
}

TEST(Span, empty_container)
{
    std::vector<int> v{1, 2, 3};
    v.clear();

    intelli::Span<int> span{v};
    intelli::View<int> view{v};

    EXPECT_FALSE(span.null());
    EXPECT_TRUE(span.empty());
    EXPECT_EQ(span.size(), 0);
    EXPECT_EQ(span.begin(), span.end());

    EXPECT_FALSE(view.null());
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0);
    EXPECT_EQ(view.begin(), view.end());
}

TEST(Span, pass_to_function)
{
    auto incr = [](intelli::Span<int> s){
        for (int& e : s) e++;
    };

    auto sum = [](intelli::View<int> s){
        int acc = 0;
        for (const auto& e: s) acc += e;
        return acc;
    };

    QVector<int> qv{1, 2, 3};
    std::array<int, 3> sa{10, 20, 30};

    EXPECT_EQ(sum(qv), 6);
    EXPECT_EQ(sum(sa), 60);

    incr(qv);
    incr(sa);

    EXPECT_EQ(sum(qv), 9);
    EXPECT_EQ(sum(sa), 63);
}
