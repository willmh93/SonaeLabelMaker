import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: colorPickerDialog
    modal: true
    width: 300
    height: 400
    title: "Select a Color"
    visible: false  // Start hidden

    signal colorSelected(color chosenColor)

    Column {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            width: 280
            height: 200
            border.color: "black"
            border.width: 1
            color: selectedColor
        }

        Slider {
            id: hueSlider
            from: 0
            to: 360
            value: 180
            onValueChanged: updateColor()
        }

        Slider {
            id: satSlider
            from: 0
            to: 1
            value: 1
            stepSize: 0.01
            onValueChanged: updateColor()
        }

        Slider {
            id: lightSlider
            from: 0
            to: 1
            value: 0.5
            stepSize: 0.01
            onValueChanged: updateColor()
        }

        Button {
            text: "Confirm"
            onClicked: {
                colorPickerDialog.colorSelected(selectedColor)
                colorPickerDialog.close()
            }
        }
    }

    function updateColor() {
        selectedColor = Qt.hsla(hueSlider.value / 360, satSlider.value, lightSlider.value, 1.0)
    }

    function openDialog() {
        colorPickerDialog.visible = true;  // Ensure dialog opens
    }
}
