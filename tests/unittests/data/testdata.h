/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef TESTNODEDATA_H
#define TESTNODEDATA_H

#include <intelli/nodedata.h>
#include <intelli/data/list.h>

#include <QVector>


class TestNodeData : public intelli::NodeData
{
    Q_OBJECT

public:

    static void registerOnce();

    Q_INVOKABLE TestNodeData(double value = {});

    /// getter for value
    Q_INVOKABLE double myDouble() const;

    /// getter to test argument forwarding
    Q_INVOKABLE double myDoubleModified(int i, QString const& s) const;

    /// setter
    Q_INVOKABLE void setMyDouble(double value);

private:

    double m_value;
};

/**
 * @brief Specialized list type storing TestNodeData elements. Used to test the
 * specialized list type path of the node data factory.
 */
class TestListData : public intelli::ListData
{
    Q_OBJECT

public:

    Q_INVOKABLE TestListData();

    /// Returns the associated list type id
    QString typeId() const override;

    /// Appends the data to the list. Returns whether appending was successfull
    bool append(intelli::NodeDataPtr const& data) override;

protected:

    /// Returns the size of the underlying list
    size_t getSize() const noexcept override;

    /// Returns the element at the given position
    intelli::NodeDataPtr getAt(size_t idx) const override;

private:

    QVector<intelli::Ptr<TestNodeData>> m_data;
};

GT_INTELLI_DECLARE_LIST_TYPE(TestNodeData, TestListData);

#endif // TESTNODEDATA_H
