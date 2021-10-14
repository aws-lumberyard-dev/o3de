from PySide2 import QtWidgets, QtCore
from PySide2.QtCore import Signal, Slot
import logging

_LOGGER = logging.getLogger('Launcher.navigation')


class Navigation(QtWidgets.QWidget):
    section_requested = Signal(str)

    def __init__(self, model):
        super(Navigation, self).__init__()

        self.model = model
        self.button_container = QtWidgets.QGridLayout()
        self.section_buttons = {}
        self.setLayout(self.button_container)
        self.button_container.setContentsMargins(0, 0, 0, 0)
        self.button_container.setSpacing(2)

        for index, section in enumerate(self.model.get_sections()):
            section_button = NavigationButton(section)
            section_button.clicked.connect(self.navigation_button_clicked)
            self.section_buttons[section] = section_button
            self.button_container.addWidget(section_button, 0, index)

    @Slot(str)
    def navigation_button_clicked(self, target_section):
        self.section_requested.emit(target_section)
        for key, value in self.section_buttons.items():
            if key.lower() == target_section:
                value.set_button_state('active')
            else:
                value.set_button_state('enabled')


class NavigationButton(QtWidgets.QWidget):
    clicked = Signal(str)

    def __init__(self, section):
        super(NavigationButton, self).__init__()

        self.section = section
        self.active = False
        self.button_frame = QtWidgets.QFrame(self)
        self.button_frame.setGeometry(0, 0, 1000, 1000)
        self.button_frame.setStyleSheet('background-color:rgb(35, 35, 35);')
        self.hit = QtWidgets.QLabel(self.section)
        self.hit.setStyleSheet('color: white')
        self.hit.setAlignment(QtCore.Qt.AlignCenter)
        self.hit.setProperty('index', 1)
        self.hit.setFixedHeight(60)
        self.hit.installEventFilter(self)
        self.layout = QtWidgets.QVBoxLayout(self)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.addWidget(self.hit)

    def set_button_state(self, state: str):
        if state == 'active':
            _LOGGER.info(f'{self.section} button... set to {state}')
            self.hit.setStyleSheet('font-weight: bold; color: red')
            self.active = True
        else:
            self.hit.setStyleSheet('font-weight: regular; color: white;')
            self.active = False

    def eventFilter(self, obj: QtWidgets.QLabel, event: QtCore.QEvent) -> bool:
        if isinstance(obj, QtWidgets.QWidget):
            if event.type() == QtCore.QEvent.MouseButtonPress:
                self.clicked.emit(self.section.lower())
            elif event.type() == QtCore.QEvent.Enter:
                if not self.active:
                    self.hit.setStyleSheet('font-weight: bold;')
            elif event.type() == QtCore.QEvent.Leave:
                if not self.active:
                    self.hit.setStyleSheet('font-weight: regular;')
        return super(NavigationButton, self).eventFilter(obj, event)
