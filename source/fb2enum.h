#ifndef FB2ENUM_H
#define FB2ENUM_H

namespace Fb {

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
    TextFind,
    TextReplace,
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
    InsertLine,
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
    ViewInspect,
    ZoomIn,
    ZoomOut,
    ZoomReset,
};

}

#endif // FB2ENUM_H
