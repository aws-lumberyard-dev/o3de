/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/MaterialEditor/Window/SettingsDialog/MaterialEditorSettingsDialog.h>
#include <Editor/Source/MaterialEditor/Window/SettingsDialog/MaterialEditorSettingsWidget.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace PhysX
{
    SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("PhysX Material Editor Settings");
        setFixedSize(600, 300);
        setLayout(new QVBoxLayout(this));

        auto settingsWidget = new SettingsWidget(this);
        settingsWidget->Populate();
        layout()->addWidget(settingsWidget);

        // Create the bottom row of the dialog with action buttons
        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        layout()->addWidget(buttonBox);

        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setModal(true);
    }
} // namespace PhysX
