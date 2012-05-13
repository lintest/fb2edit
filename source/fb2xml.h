#ifndef FB2XML_H
#define FB2XML_H

#include <QHash>

#define FB2_BEGIN_KEYLIST private: enum Keyword {

#define FB2_END_KEYLIST None }; \
class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); }; \
static Keyword toKeyword(const QString &name); private:

#define FB2_BEGIN_KEYHASH(x) \
x::Keyword x::toKeyword(const QString &name) \
{                                                                    \
    static const KeywordHash map;                                    \
    KeywordHash::const_iterator i = map.find(name);                  \
    return i == map.end() ? None : i.value();                        \
}                                                                    \
x::KeywordHash::KeywordHash() {

#define FB2_END_KEYHASH }

#define FB2_KEY(key,str) insert(str,key);

#endif // FB2XML_H
