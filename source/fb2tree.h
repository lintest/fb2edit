#ifndef FB2TREE_H
#define FB2TREE_H

#include <QAbstractItemModel>
#include <QTextDocument>

class Fb2TreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Fb2TreeModel(QTextDocument &document, QObject *parent = 0);
    virtual ~Fb2TreeModel();

private:
    QTextDocument & m_document;
};

#endif // FB2TREE_H
