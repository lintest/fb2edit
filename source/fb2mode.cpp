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
        action->setEnabled(false);
        action->setChecked(false);
        action->disconnect();
    }
}

