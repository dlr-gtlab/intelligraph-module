/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef NDS_NODESMODULE_H
#define NDS_NODESMODULE_H

#include "gt_moduleinterface.h"
#include "gt_datamodelinterface.h"
#include "gt_processinterface.h"
#include "gt_mdiinterface.h"
#include "gt_propertyinterface.h"

/**
 * @generated 1.2.0
 * @brief The NdsNodesModule class
 */
class NdsNodesModule : public QObject,
        public GtModuleInterface,
        public GtDatamodelInterface,
        public GtProcessInterface,
        public GtMdiInterface,
        public GtPropertyInterface
{
    Q_OBJECT

    GT_MODULE("nds_nodesmodule.json")

    Q_INTERFACES(GtModuleInterface)
    Q_INTERFACES(GtDatamodelInterface)
    Q_INTERFACES(GtProcessInterface)
    Q_INTERFACES(GtMdiInterface)
    Q_INTERFACES(GtPropertyInterface)
    
public:

    /**
     * @brief Returns current version number of module
     * @return version number
     */
    GtVersionNumber version() override;

    /**
     * @brief Returns module description
     * @return description
     */
    QString description() const override;

    /**
     * @brief Initializes module. Called on application startup.
     */
    void init() override;

    /**
     * @brief Passes additional module information to the framework.
     *
     * NOTE: A reference to the author can significantly help the user to
     * know who to contact in case of issues or other request.
     * @return module meta information.
     */
    MetaInformation metaInformation() const override;

    /**
     * @brief Returns static meta objects of datamodel package.
     * @return package meta object
     */
    QMetaObject package() override;

    /**
     * @brief Returns static meta objects of datamodel classes.
     * @return list including meta objects
     */
    QList<QMetaObject> data() override;

    /**
     * @brief Returns true if module is a standalone module with own data
     * model structure. Otherwise module only extends the overall application
     * with additional functionalities like classes, calculators or 
     * graphical user interfaces.
     * @return Standalone indicator.
     */
    bool standAlone() override;

    /**
     * @brief Returns static meta objects of calculator classes.
     * @return list of meta objects.
     */
    QList<GtCalculatorData> calculators() override;

    /**
     * @brief Returns static meta objects of task classes.
     * @return list of meta objects.
     */
    QList<GtTaskData> tasks() override;

    /**
     * @brief Returns static meta objects of mdi item classes.
     * @return list including meta objects
     */
    QList<QMetaObject> mdiItems() override;

    /**
     * @brief Returns static meta objects of dockwidget classes.
     * @return list including meta objects
     */
    QList<QMetaObject> dockWidgets() override;

    /**
     * @brief uiItems
     * @return data class names mapped to ui item objects
     */
    QMap<const char*, QMetaObject> uiItems() override;

    /**
     * @brief postItems
     * @return
     */
    QList<QMetaObject> postItems() override;

    /**
     * @brief postPlots
     * @return
     */
    QList<QMetaObject> postPlots() override;

    /**
     * @brief Returns static meta objects of property item classes.
     * @return list of meta objects of property item classes.
     */
    QMap<const char*, QMetaObject> propertyItems() override;;
};

#endif // NDS_NODESMODULE_H
