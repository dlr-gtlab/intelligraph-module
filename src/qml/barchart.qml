import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15
import QtGraphicalEffects 1.15

Rectangle {
        id: rect
        implicitWidth: 400
        implicitHeight: 300
        color: "#152231"

        ChartView {
            title: "Stacked Bar series"
            anchors.fill: parent
            legend.alignment: Qt.AlignBottom
            antialiasing: true
            theme: ChartView.ChartThemeBlueIcy

            StackedBarSeries {
                id: mySeries
                axisX: BarCategoryAxis { categories: ["2007", "2008", "2009", "2010", "2011", "2012" ] }
                BarSet { label: "Bob"; values: [2, 2, 3, 4, 5, 6] }
                BarSet { label: "Susan"; values: [5, 1, 2, 4, 1, 7] }
                BarSet { label: "James"; values: [3, 5, 8, 13, 5, 8] }
            }
        }
}
