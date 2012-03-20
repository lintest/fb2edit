#ifndef FB2DOC_H
#define FB2DOC_H

#include <QTextDocument>
#include <QVariant>

class Fb2ChildDocument : public QTextDocument
{
    Q_OBJECT
public:
    explicit Fb2ChildDocument(QTextDocument &parent)
        : QTextDocument(&parent), m_parent(parent) {}
protected:
    virtual QVariant loadResource(int type, const QUrl &name)
        { return m_parent.resource(type, name); }
private:
    QTextDocument &m_parent;
};

class Fb2MainDocument : public QTextDocument
{
    Q_OBJECT
public:
    explicit Fb2MainDocument(QObject *parent = 0) : QTextDocument(parent), m_child(*this) {}
    Fb2ChildDocument & child() { return m_child; }
private:
    Fb2ChildDocument m_child;
};

#endif // FB2DOC_H
