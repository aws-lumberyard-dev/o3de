/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <QDir>
#include <QProcess>

namespace AzQtComponents
{
    void ShowFileOnDesktop(const QString& path)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }

    QString fileBrowserActionName()
    {
        const char* exploreActionName = "Open in file browser";
        return QObject::tr(exploreActionName);
    }
}
