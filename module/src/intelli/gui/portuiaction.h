/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_PORTUIACTION_H
#define GT_INTELLI_PORTUIACTION_H

#include <intelli/node.h>

#include <functional>

#include <QIcon>

namespace intelli
{

/**
 * @brief The GtIgPortUIAction class
 * Holds the data for a single port action
 */
class PortUIAction
{
public:
    
    using ActionMethod       = std::function<void (Node*, PortType, PortIndex)>;
    using VerificationMethod = std::function<bool (Node*, PortType, PortIndex)>;
    using VisibilityMethod   = std::function<bool (Node*, PortType, PortIndex)>;

    PortUIAction() = default;

    PortUIAction(QString text, ActionMethod method) :
        m_text(std::move(text)), m_method(std::move(method))
    { }

    bool empty() const { return m_text.isEmpty() || !m_method; }

    /* @brief text
     * @return
     */
    const QString& text() const { return m_text; }

    /**
     * @brief icon
     * @return
     */
    const QIcon& icon() const { return m_icon; }

    /**
     * @brief Action method. Must be called with a parent and target objet as
     * parameters
     * @return Action method
     */
    const ActionMethod& method() const { return m_method; }

    /**
     * @brief Verification method used to check if action should be enabled.
     * Must be called with a parent and target objet as
     * parameters
     * @return Verification method
     */
    const VerificationMethod& verificationMethod() const { return m_verification; }

    /**
     * @brief Visibility method used to check if action should be visible.
     * Must be called with a parent and target objet as
     * parameters
     * @return Visibility method
     */
    const VisibilityMethod& visibilityMethod() const { return m_visibility; }

    /**
     * @brief Dedicated setter for the UI icon
     * @param icon Icon
     * @return This
     */
    PortUIAction& setIcon(const QIcon& icon) { m_icon = icon; return *this; }

    /**
     * @brief Dedicated setter for the verification method.
     * Function signature must accept a pointer of the target
     * object.
     * @param method Method
     * @return This
     */
    PortUIAction& setVerificationMethod(VerificationMethod method)
    {
        m_verification = std::move(method); return *this;
    }

    /**
     * @brief Dedicated setter for the visibility method. Function signature must accept a pointer of the target
     * object.
     * @param method Method
     * @return This
     */
    PortUIAction& setVisibilityMethod(VisibilityMethod method)
    {
        m_visibility = std::move(method); return *this;
    }

private:

    /// Action text
    QString m_text{};

    /// Action icon
    QIcon m_icon{};

    /// Action method
    ActionMethod m_method{};

    /// Verification method
    VerificationMethod m_verification{};

    /// Visibility method
    VisibilityMethod m_visibility{};
};

} // namespace intelli

#endif // GT_INTELLI_PORTUIACTION_H