from PySide2 import QtWidgets
import logging
import os

_LOGGER = logging.getLogger('Launcher.setup')


class Setup(QtWidgets.QWidget):
    def __init__(self, model):
        super(Setup, self).__init__()

        _LOGGER.info('Setup Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(0, 0, 100);')
        self.info = QtWidgets.QTextEdit()
        self.info.setFixedWidth(400)
        self.content_layout.addWidget(self.info)
        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Setup Section...')
        info_text = 'Setup Information:\n\nFound environment variables:\n'
        for index, value in os.environ.items():
            info_text += f'-- {index}: {value}\n'
        self.set_info(info_text)

    def set_info(self, info_text: str):
        self.info.setText(info_text)

    def open_section(self):
        pass

    def close_section(self):
        pass
