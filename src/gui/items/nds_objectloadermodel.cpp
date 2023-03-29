#include <QEvent>

#include "gt_application.h"
#include "gt_project.h"
#include "gt_objectselectiondialog.h"

#include "nds_objectdata.h"

#include "nds_objectloadermodel.h"

NdsObjectLoaderModel::NdsObjectLoaderModel() :
    _label(new QLabel("Select Object...")),
    m_obj(nullptr)
{
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    QFont f = _label->font();
    f.setBold(true);
    f.setItalic(true);

    _label->setFont(f);

    _label->setMinimumSize(100, 50);
//    _label->setMaximumSize(500, 300);

    _label->installEventFilter(this);
}

unsigned int
NdsObjectLoaderModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 0;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

bool
NdsObjectLoaderModel::eventFilter(QObject* object, QEvent* event)
{
    if (object == _label) {
        int w = _label->width();
        int h = _label->height();

        if (event->type() == QEvent::MouseButtonPress) {

            QStringList list{"GtpFlowStart", "GtpFlowEnd", "GtdRotorBladeRow", "GtdStatorBladeRow", "GtdDisk"};

            GtObjectSelectionDialog dialog(gtApp->currentProject());
            dialog.setFilterData(list);

            if (dialog.exec())
            {
                GtObject* obj = dialog.currentObject();

                if (obj)
                {
                    m_obj = obj;
                    _label->setText(obj->objectName());
                    Q_EMIT dataUpdated(0);
                }
            }

            return true;
        }/* else if (event->type() == QEvent::Resize) {
            if (!_pixmap.isNull())
                _label->setPixmap(_pixmap.scaled(w, h, Qt::KeepAspectRatio));
        }*/
    }

    return false;
}

NodeDataType
NdsObjectLoaderModel::dataType(PortType const, PortIndex const) const
{
    return NdsObjectData().type();
}

std::shared_ptr<NodeData>
NdsObjectLoaderModel::outData(PortIndex)
{
    return std::make_shared<NdsObjectData>(m_obj);
}
