/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

/*
 * generated 1.2.0
 */
 
#include "intelli/package.h"
#include "intelli/graph.h"
#include "intelli/graphcategory.h"

#include <gt_objectmemento.h>
#include <gt_objectfactory.h>
#include <gt_coreapplication.h>
#include <gt_project.h>
#include <gt_qtutilities.h>

#include <gt_logging.h>

#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>

using namespace intelli;

QString const& MODULE_DIR = QStringLiteral("intelli");

QString const& FILE_SUFFIX = QStringLiteral(".gtflow");

Package::Package()
{
//    setFlag(UserHidden);

    setObjectName("IntelliGraphs");
}

bool
Package::readData(const QDomElement& root)
{
    assert(gtObjectFactory);
    assert(gtApp);

    auto const makeError = [](){
        return tr("Failed to read package data!");
    };

    auto* project = gtApp->currentProject();

    if (!project)
    {
        gtError() << makeError() << tr("(project could not be accessed)");
        gtError() << tr("The GTlab version is most likely incompatible with the module version of '%1'! "
                        "Reading from module file instead...")
                   .arg(GT_MODULENAME());
        return GtPackage::readData(root);
    }

    QDir dir = project->path();

    if (!dir.cd(MODULE_DIR) || !dir.exists())
    {
        gtInfo().verbose()
            << tr("Module dir '%1' does not exists. Reading from module file instead...").arg(MODULE_DIR);
        return GtPackage::readData(root);
    }

    if (dir.isEmpty(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        gtInfo().verbose()
            << tr("Empty module '%1' dir. Reading from module file instead...").arg(MODULE_DIR);
        return GtPackage::readData(root);
    }

    QDirIterator iter{
        dir.path(),
        QStringList{QStringLiteral("*")},
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags
    };

    bool success = true;

    while (iter.hasNext())
    {
        iter.next();
        QString const& name = iter.fileName();
        gtDebug() << name;

        QDir catDir = dir;
        if (!catDir.cd(name))
        {
            gtError() << makeError()
                      << tr("(category dir '%1' could not be accessed)").arg(name);
            success = false;
            continue;
        }

        auto cat = std::make_unique<GraphCategory>();
        cat->setObjectName(name);

        if (!appendChild(cat.get()))
        {
            gtError() << makeError()
                      << tr("(category '%1' could not be loaded)").arg(name);
            success = false;
            continue;
        }

        QDirIterator fileIter{
            catDir.path(),
            QStringList{'*' + FILE_SUFFIX},
            QDir::Files,
            QDirIterator::NoIteratorFlags
        };

        while (fileIter.hasNext())
        {
            QString const& fileName = fileIter.next();

            QFile file(catDir.absoluteFilePath(fileName));

            if (!file.open(QIODevice::ReadOnly))
            {
                gtError() << makeError()
                          << tr("(graph flow '%1' could not be opend!)")
                                 .arg(fileName);
                success = false;
                continue;
            }

            GtObjectMemento memento(file.readAll());

            auto graph = gt::unique_qobject_cast<Graph>(memento.toObject(*gtObjectFactory));

            if (!graph || !cat->appendChild(graph.get()))
            {
                gtError() << makeError()
                          << tr("(graph flow '%1' could not be opend!)")
                                 .arg(fileName);
                success = false;
                continue;
            }

            graph.release();
        }

        cat.release();
    }

    return success;
}

bool
Package::saveData(QDomElement& root, QDomDocument& doc)
{
    assert(gtApp);

    auto const makeError = [](){
        return tr("Failed to save package data!");
    };

    auto* project = gtApp->currentProject();

    if (!project)
    {
        gtError() << makeError()
                  << tr("(project could not be accessed)");
        GtPackage::saveData(root, doc);
        return false;
    }

    auto const& categories = findDirectChildren<GraphCategory*>();
    if (categories.empty())
    {
        gtInfo().verbose()
            << tr("Not saving module '%1', no data to save!").arg(GT_MODULENAME());
        return true;
    }

    QDir dir = project->path();

    dir.mkdir(MODULE_DIR);
    if (!dir.cd(MODULE_DIR))
    {
        gtError() << makeError()
                  << tr("(module dir '%1' could not be created)").arg(MODULE_DIR);
        GtPackage::saveData(root, doc);
        return false;
    }

    bool success = true;

    for (auto const* cat : categories)
    {
        auto const& name = cat->objectName();

        QDir catDir = dir;

        catDir.mkdir(name);
        if (!catDir.cd(name))
        {
            gtError() << makeError()
                      << tr("(category dir '%1' could not be created)").arg(name);
            success = false;
            continue;
        }

        QDirIterator fileIter{
            catDir.path(),
            QStringList{'*' + FILE_SUFFIX},
            QDir::Files,
            QDirIterator::NoIteratorFlags
        };

        while (fileIter.hasNext())
        {
            catDir.remove(fileIter.next());
        }

        auto const& graphs = cat->findDirectChildren<Graph*>();

        for (auto const* graph : graphs)
        {
            auto const& fileName = graph->uuid() + FILE_SUFFIX;

            QFile file(catDir.absoluteFilePath(fileName));

            if (!file.open(QIODevice::Truncate | QIODevice::ReadWrite))
            {
                gtError() << makeError()
                          << tr("(graph flow '%1' (%2) could not be created!)")
                                 .arg(fileName, graph->caption());
                success = false;
                continue;
            }

            auto data = graph->toMemento().toByteArray();

            if (file.write(data) != data.size())
            {
                gtError() << makeError()
                          << tr("(graph flow '%1' (%2) could not be saved!)")
                                 .arg(fileName, graph->caption());
                success = false;
                continue;
            }
        }
    }

    return success;
}
