#ifndef GT_IGSHAPECOLORNODE_H
#define GT_IGSHAPECOLORNODE_H

#include <QColor>

#include "gt_igabstractshapenode.h"
#include "gt_igcolorporperty.h"

class GtIgShapeColorNode : public GtIgAbstractShapeNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgShapeColorNode();

    QWidget* embeddedWidget() override
    {
        if (!m_editor) initWidget();
        return m_editor;
    }

protected:

    bool eventFilter(QObject* object, QEvent* event) override;

    void compute(QList<ShapePtr> const& shapesIn,
                 QList<ShapePtr>& shapesOut) override;

private:

    GtIgColorPorperty m_color;

    gt::ig::volatile_ptr<QWidget> m_editor;

    void setWidgetColor();

    void initWidget();
};

#endif // GT_IGSHAPECOLORNODE_H
