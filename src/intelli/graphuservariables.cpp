/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/graphuservariables.h>
#include <intelli/private/utils.h>

#include <gt_coreapplication.h>
#include <gt_propertystructcontainer.h>
#include <gt_structproperty.h>
#include <gt_variantproperty.h>
#include <gt_variantconvert.h>
#include <gt_mpl.h>

#include <QVariant>

using namespace intelli;

struct GraphUserVariables::Impl
{
    GtPropertyStructContainer variables{
        "userVars", tr("User Variables")
    };
};

QString const& S_TYPE   = QStringLiteral("Entry");
QString const& S_MEMBER = QStringLiteral("value");

GraphUserVariables::GraphUserVariables(GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("__user_variables"));

    setFlag(UserRenamable, false);
    setFlag(UserDeletable, false);
    setUserHidden(!(gtApp && gtApp->devMode()));

    GtPropertyStructDefinition def{S_TYPE};
    def.defineMember(S_MEMBER, gt::makeVariantProperty());

    pimpl->variables.registerAllowedType(def);
    registerPropertyStructContainer(pimpl->variables);
}

GraphUserVariables::~GraphUserVariables() = default;

namespace
{

/// Operator that returns template argument as is
struct identity
{
    template <typename T>
    T&& operator()(T&& t) const { return t; }
};

/// Helper function that checks if variant `v` is compatible to any `T`.
/// Prints error message if not.
template<typename ...T>
bool checkCompatibility(QVariant const& v)
{
    std::initializer_list<bool> convertible{gt::canConvert<T>(v)...};

    if (std::none_of(convertible.begin(), convertible.end(), identity{}))
    {
        gt::log::Stream s;
        s << QObject::tr("GraphUserVariables: "
                         "Unsupported variant type, must be one of: '");

        // iterate over types and print names
        std::tuple<T...> tuple{};
        gt::mpl::static_foreach(tuple, [&s](auto type){
            s << gt::log::nospace
              << '"' << gt::mpl::type_name<decltype(type)>::get() << "\" ";
        });
        s << gt::log::space << "',"
          << QObject::tr("got: '%1'").arg(toString(v));

        gtWarning() << s.str();
        return false;
    }

    return true;
}

} // namespace

bool
GraphUserVariables::setValue(QString const& key, QVariant const& value)
{
    if (!checkCompatibility<
            bool,
            int,
            unsigned,
            double,
            QString
            >(value)) {
        return false;
    }

    // overwrite value
    auto iter = pimpl->variables.findEntry(key);
    if (iter != pimpl->variables.end())
    {
        iter->setMemberVal(S_MEMBER, value);
        return true;
    }
    // add new value
    GtPropertyStructInstance& instance = pimpl->variables.newEntry(S_TYPE, key);
    instance.setMemberVal(S_MEMBER, value);
    return true;
}

bool
GraphUserVariables::remove(QString const& key)
{
    auto iter = pimpl->variables.findEntry(key);
    if (iter == pimpl->variables.end()) return false;
    pimpl->variables.removeEntry(iter);
    return true;
}

bool
GraphUserVariables::hasValue(QString const& key) const
{
    return pimpl->variables.findEntry(key) != pimpl->variables.end();
}

QVariant
GraphUserVariables::value(QString const& key) const
{
    auto iter = pimpl->variables.findEntry(key);
    if (iter == pimpl->variables.end()) return {};

    return iter->getMemberValToVariant(S_MEMBER);
}

QStringList
GraphUserVariables::keys() const
{
    QStringList list;
    list.reserve(size());
    visit([&](QString const& key, QVariant const&){
        list.push_back(key);
    });
    return list;
}

size_t
GraphUserVariables::size() const
{
    return pimpl->variables.size();
}

void
GraphUserVariables::visit(std::function<void (const QString&, const QVariant&)> f) const
{
    for (GtPropertyStructInstance& entry : pimpl->variables)
    {
        f(entry.ident(), entry.getMemberValToVariant(S_MEMBER));
    }
}

