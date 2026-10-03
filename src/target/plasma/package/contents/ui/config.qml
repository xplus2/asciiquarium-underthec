import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.underthec 1.0

Kirigami.FormLayout {
    id: root
    twinFormLayouts: parentLayout

    property alias formLayout: root
    property int cfg_Classic
    property bool cfg_FishAuto
    property int cfg_Fish
    property var cfg_Creatures: []
    property string cfg_Message
    property string cfg_MessageColor
    property string cfg_MessagePosition
    property bool cfg_CastleEnabled
    property string cfg_CastleName
    property double cfg_Pace
    property int cfg_UturnChance
    property int cfg_Fps
    property int cfg_FontSize
    property string cfg_Colors

    readonly property var colorValues: ["", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"]
    readonly property var positionValues: ["middle", "center", "marquee", "swim", "event"]
    readonly property var colorsValues: ["1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
                                         "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
                                         "4", "7", "8", "8-bold", "16"]
    readonly property var colorsLabels: ["1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
                                         "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
                                         "4 (RGBW)", "7 (Teletext)", "8", "8-bold", "16 (ANSI)"]
    readonly property bool freeLife: cfg_Classic === 0

    UnderTheCInfo {
        id: info
    }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Classic mode:")
        model: [i18n("Off"), "1.0", "1.1"]
        currentIndex: root.cfg_Classic
        onActivated: index => root.cfg_Classic = index
    }

    QQC2.CheckBox {
        Kirigami.FormData.label: i18n("Fish:")
        text: i18n("Automatic count")
        enabled: root.freeLife
        checked: root.cfg_FishAuto
        onToggled: root.cfg_FishAuto = checked
    }

    QQC2.SpinBox {
        Kirigami.FormData.label: i18n("Fish count:")
        from: 0
        to: 999
        enabled: root.freeLife && !root.cfg_FishAuto
        value: root.cfg_Fish
        onValueModified: root.cfg_Fish = value
    }

    GridLayout {
        Kirigami.FormData.label: i18n("Creatures:")
        Kirigami.FormData.labelAlignment: Qt.AlignTop
        columns: 2
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: 0

        Repeater {
            model: info.creatures

            QQC2.CheckBox {
                required property string modelData
                text: modelData
                enabled: root.freeLife
                checked: root.cfg_Creatures.indexOf(modelData) >= 0
                onToggled: {
                    var list = root.cfg_Creatures.slice();
                    var i = list.indexOf(modelData);
                    if (checked && i < 0) list.push(modelData);
                    if (!checked && i >= 0) list.splice(i, 1);
                    root.cfg_Creatures = list;
                }
            }
        }
    }

    FontMetrics {
        id: messageMetrics
        font: Kirigami.Theme.fixedWidthFont
    }

    QQC2.ScrollView {
        Kirigami.FormData.label: i18n("Message:")
        implicitWidth: messageMetrics.averageCharacterWidth * 40 + messageArea.leftPadding + messageArea.rightPadding
        implicitHeight: messageMetrics.lineSpacing * 4 + messageArea.topPadding + messageArea.bottomPadding

        QQC2.TextArea {
            id: messageArea
            font: Kirigami.Theme.fixedWidthFont
            wrapMode: TextEdit.NoWrap
            text: root.cfg_Message
            onTextChanged: if (text !== root.cfg_Message) root.cfg_Message = text
        }
    }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Message color:")
        model: [i18n("(default)")].concat(root.colorValues.slice(1))
        currentIndex: root.colorValues.indexOf(root.cfg_MessageColor)
        onActivated: index => root.cfg_MessageColor = root.colorValues[index]
    }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Message position:")
        model: root.positionValues
        currentIndex: root.positionValues.indexOf(root.cfg_MessagePosition)
        onActivated: index => root.cfg_MessagePosition = root.positionValues[index]
    }

    QQC2.CheckBox {
        Kirigami.FormData.label: i18n("Castle:")
        text: i18n("Show castle")
        checked: root.cfg_CastleEnabled
        onToggled: root.cfg_CastleEnabled = checked
    }

    QQC2.TextField {
        Kirigami.FormData.label: i18n("Castle name:")
        enabled: root.cfg_CastleEnabled
        text: root.cfg_CastleName
        onTextEdited: root.cfg_CastleName = text
    }

    QQC2.SpinBox {
        id: paceBox
        Kirigami.FormData.label: i18n("Pace:")
        from: 1
        to: 1000
        editable: true
        value: Math.round(root.cfg_Pace * 100)
        textFromValue: (v, locale) => (v / 100).toFixed(2)
        valueFromText: (t, locale) => Math.round(parseFloat(t) * 100)
        onValueModified: root.cfg_Pace = value / 100
    }

    QQC2.SpinBox {
        Kirigami.FormData.label: i18n("U-turn chance:")
        from: 0
        to: 999
        value: root.cfg_UturnChance
        onValueModified: root.cfg_UturnChance = value
    }

    QQC2.SpinBox {
        Kirigami.FormData.label: i18n("Frames per second:")
        from: 1
        to: 240
        value: root.cfg_Fps
        onValueModified: root.cfg_Fps = value
    }

    QQC2.SpinBox {
        Kirigami.FormData.label: i18n("Font size:")
        from: 0
        to: 200
        value: root.cfg_FontSize
        textFromValue: (v, locale) => v === 0 ? i18n("Automatic") : v + " px"
        valueFromText: (t, locale) => parseInt(t) || 0
        onValueModified: root.cfg_FontSize = value
    }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Colors:")
        model: root.colorsLabels
        currentIndex: root.colorsValues.indexOf(root.cfg_Colors)
        onActivated: index => root.cfg_Colors = root.colorsValues[index]
    }
}
