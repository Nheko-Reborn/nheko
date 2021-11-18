#ifndef JDENTICONINTERFACE_H
#define JDENTICONINTERFACE_H

#include <QString>

class JdenticonInterface
{
public:
    virtual ~JdenticonInterface() {}
    virtual QString generate(const QString &message, uint16_t size) = 0;
};

#define JdenticonInterface_iid "im.nheko.JdenticonInterface"

Q_DECLARE_INTERFACE(JdenticonInterface, JdenticonInterface_iid)

#endif // JDENTICONINTERFACE_H
