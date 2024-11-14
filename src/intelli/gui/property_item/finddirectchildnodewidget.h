/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 07.06.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef FINDDIRECTCHILDNODEWIDGET_H
#define FINDDIRECTCHILDNODEWIDGET_H

#include <QWidget>

#include "intelli/data/object.h"

class GtLineEdit;

namespace intelli
{

/**
 * @brief The FindDirectChildNodeWidget class
 * The widget for the find-direct child node
 */
class FindDirectChildNodeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief FindDirectChildNodeWidget
     * Constructor to define the basic structure of the widget and its elements
     * @param parent - optional parent widget of the widget
     */
    Q_INVOKABLE FindDirectChildNodeWidget(QWidget* parent = nullptr);

    /**
     * @brief setClassNameWidget sets the class name widget to the given string
     * @param className
     */
    void setClassNameWidget(QString const& className);

    /**
     * @brief setObjectNameWidget sets the object name widget to the given string
     * @param objectName
     */
    void setObjectNameWidget(QString const& objectName);
public slots:
    /**
     * @brief updateNameCompleter
     * Changes the completer based on the names of the dhildren of
     * the given object
     * @param data
     */
    void updateNameCompleter(std::shared_ptr<const ObjectData> data);

    void reactOnClassNameWidgetChange();

    void reactOnObjectNameWidgetChange();

    void updateClassText();

    void updateNameText();

private:
    GtLineEdit* m_objectNameEdit{nullptr};

    GtLineEdit* m_classNameEdit{nullptr};

signals:
    void updateClass(QString newClassName);

    void updateObjectName(QString newObjectName);
};

} /// namespace intelli
#endif // FINDDIRECTCHILDNODEWIDGET_H
