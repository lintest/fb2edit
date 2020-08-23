#include "fb2mode.h"

//---------------------------------------------------------------------------
//  FbActionMap
//---------------------------------------------------------------------------

void FbActionMap::connect()
{
    for (QAction *action: *this) {
        action->setEnabled(true);
    }
}

void FbActionMap::disconnect()
{
    for (QAction *action: *this) {
        if (action->isCheckable()) action->setChecked(false);
        action->setEnabled(false);
        action->disconnect();
    }
}

