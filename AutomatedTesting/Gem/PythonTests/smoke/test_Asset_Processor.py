"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
LY-124067: UI Apps: AssetProcessor
Open AssetProcessor, Wait until it is launched.
Close AssetProcessor.
"""

# Import builtin libraries
import pytest
import os
import sys

# Import LyTestTools
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/../assetpipeline/")

# Import fixtures
from ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ap_fixtures.ap_idle_fixture import TimestampChecker


@pytest.fixture
def ap_idle(workspace, ap_setup_fixture):
    # AP should process all assets within 60 seconds
    gui_timeout_max = 30
    return TimestampChecker(
        workspace.paths.asset_catalog(ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]), gui_timeout_max
    )


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.usefixtures("automatic_process_killer")
# Asset Processor should process all assets within 60 seconds. Test case will fail, If Asset Processor takes more than 60 seconds
@pytest.mark.timeout(60)
@pytest.mark.SUITE_smoke
class TestAssetProcessor(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, asset_processor):
        def teardown():
            asset_processor.stop()

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("LY-124067")
    def test_Asset_Processor(self, asset_processor, workspace, ap_idle):
        # Create an asset root
        asset_processor.create_temp_asset_root()
        # Run batch process to ensure project assets are processed
        assert asset_processor.batch_process(), "AP Batch failed"
        ap_idle.set_file_path(workspace.paths.ap_gui_log())
        # Launch Asset Processor and wait for it to go idle
        result, _ = asset_processor.gui_process()
        assert result, "AP failed"
