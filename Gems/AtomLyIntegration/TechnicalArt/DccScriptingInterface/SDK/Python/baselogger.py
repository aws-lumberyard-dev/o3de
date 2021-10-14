from dynaconf import settings
from azpy.constants import FRMT_LOG_LONG
from pathlib import Path
import logging
import sys
import os

for handler in logging.root.handlers[:]:
    logging.root.removeHandler(handler)


class BaseLogger:
    def __init__(self, overrides=False):
        self.level = logging.DEBUG
        self.log_levels = ('DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL')
        self.format = FRMT_LOG_LONG
        self.date_format = '%m-%d %H:%M'
        self.log_file = Path(settings.DCCSI_LOG_PATH, f'launcher.log')
        os.makedirs(os.path.dirname(self.log_file), exist_ok=True)
        self.file_handler = logging.FileHandler(filename=self.log_file)
        self.stream_handler = logging.StreamHandler(sys.stdout)
        self.handlers = [self.file_handler, self.stream_handler]
        self.file_mode = 'w'

        logging.basicConfig(level=self.level,
                            format=self.format,
                            datefmt=self.date_format,
                            handlers=self.handlers)
        _LOGGER = logging.getLogger(__name__)

        if overrides:
            _LOGGER.info('Add override code here')


BaseLogger()

