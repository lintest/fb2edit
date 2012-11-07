#ifndef FB2MODE_H
#define FB2MODE_H

#include <QAction>
#include <QMap>

namespace Fb {

enum Mode {
    Text,
    Head,
    Code,
    Html,
};

enum Actions {
    GoBack,
    GoForward,
    EditUndo,
    EditRedo,
    EditCut,
    EditCopy,
    EditPaste,
    PasteText,
    TextSelect,
    EditFind,
    EditReplace,
    CheckText,
    InsertImage,
    InsertNote,
    InsertLink,
    InsertBody,
    InsertTitle,
    InsertEpigraph,
    InsertSubtitle,
    InsertAnnot,
    InsertCite,
    InsertPoem,
    InsertDate,
    InsertStanza,
    InsertAuthor,
    InsertSection,
    InsertText,
    InsertParag,
    ClearFormat,
    TextBold,
    TextItalic,
    TextStrike,
    TextSub,
    TextSup,
    TextCode,
    TextTitle,
    SectionAdd,
    SectionDel,
    ViewContents,
    ViewPictures,
    ViewFootnotes,
    ViewInspector,
    ZoomIn,
    ZoomOut,
    ZoomReset,
};

}

class FbActionMap : public QMap<Fb::Actions, QAction*>
{
public:
    void connect();
    void disconnect();
};

#endif // FB2MODE_H
