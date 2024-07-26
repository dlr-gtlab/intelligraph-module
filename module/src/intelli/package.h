/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef GT_INTELLI_PACKAGE_H
#define GT_INTELLI_PACKAGE_H

#include <intelli/exports.h>

#include <gt_package.h>

namespace intelli
{

/**
 * @generated 1.2.0
 * @brief The GtIgPackage class
 */
class Package : public GtPackage
{
    Q_OBJECT

public:

    static QString const& MODULE_DIR;
    static QString const& FILE_SUFFIX;
    static QString const& INDEX_FILE;

    Q_INVOKABLE Package();

protected:
    /**
     * @brief Module specific data read method.
     * @param Root data.
     * @return Returns true if data was successfully read.
     */
    bool readData(const QDomElement& root);

    /**
     * @brief Module specific data save method.
     * @param Root data.
     * @return Returns true if data was successfully read.
     */
    bool saveData(QDomElement& root, QDomDocument& doc);

private:

    struct Impl;
};

} // namespace intelli

#endif // GT_INTELLI_PACKAGE_H
