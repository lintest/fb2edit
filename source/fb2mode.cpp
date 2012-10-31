#include "fb2mode.h"

//---------------------------------------------------------------------------
//  FbActionMap
//---------------------------------------------------------------------------

void FbActionMap::connect()
{
    foreach (QAction *action, *this) {
        action->setEnabled(true);
    }
}

void FbActionMap::disconnect()
{
    foreach (QAction *action, *this) {
        if (action->isCheckable()) action->setChecked(false);
        action->setEnabled(false);
        action->disconnect();
    }
}

