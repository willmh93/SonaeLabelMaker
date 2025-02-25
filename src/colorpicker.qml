import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 300
    height: 400

    Dialog {
        id: colorPickerDialog
        property color selectedColor: Qt.hsla(180 / 360, 1, 0.5, 1.0)
        modal: true
        width: 300
        height: 400
        title: "Select a Color"

        signal colorSelected(color chosenColor)

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            // Color preview rectangle
            Rectangle {
                id: colorPreview
                width: parent.width - 20
                height: 200
                border.color: "black"
                border.width: 1
                color: colorPickerDialog.selectedColor
            }

            // Hue slider
            Text { text: "Hue" }
            Slider {
                id: hueSlider
                from: 0; to: 360
                value: 180
                onValueChanged: colorPickerDialog.updateColor()
            }

            // Saturation slider
            Text { text: "Saturation" }
            Slider {
                id: satSlider
                from: 0; to: 1
                value: 1
                stepSize: 0.01
                onValueChanged: colorPickerDialog.updateColor()
            }

            // Lightness slider
            Text { text: "Lightness" }
            Slider {
                id: lightSlider
                from: 0; to: 1
                value: 0.5
                stepSize: 0.01
                onValueChanged: colorPickerDialog.updateColor()
            }

            // Confirm button to emit the selected color and close the dialog
            Button {
                text: "Confirm"
                onClicked: {
                    colorSelected(colorPickerDialog.selectedColor)
                    close()
                }
            }
        }

        function updateColor() {
            colorPickerDialog.selectedColor = Qt.hsla(hueSlider.value / 360, satSlider.value, lightSlider.value, 1.0)
        }

        // Use open() to show the dialog
        function openDialog() {
            console.log("OPENING <<<<<<<<<<");
            open();
        }

        function onClosed()
        {
            console.log("Dialog was closed!");
        }
    }
}