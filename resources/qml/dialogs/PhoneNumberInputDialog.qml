// SPDX-FileCopyrightText: 2021 Mirian Margiani
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: inputDialog

    property alias prompt: promptLabel.text
    property alias echoMode: statusInput.echoMode
    property var onAccepted: undefined

    modality: Qt.NonModal
    flags: Qt.Dialog
    width: 350
    height: fontMetrics.lineSpacing * 7

    GridLayout {
        rowSpacing: Nheko.paddingMedium
        columnSpacing: Nheko.paddingMedium
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent
        columns: 2

        Label {
            id: promptLabel

            Layout.columnSpan: 2
            color: Nheko.colors.text
        }

        ComboBox {
            id: numberPrefix

            editable: false

            delegate: ItemDelegate {
                text: n + " (" + p + ")"
            }
            // taken from https://gitlab.com/whisperfish/whisperfish/-/blob/master/qml/js/countries.js

            //n=name,i=ISO,p=prefix -- see countries.js.md for source
            model: ListModel {
                ListElement {
                    n: "Afghanistan"
                    i: "AF"
                    p: "+93"
                }

                ListElement {
                    n: "Åland Islands"
                    i: "AX"
                    p: "+358 18"
                }

                ListElement {
                    n: "Albania"
                    i: "AL"
                    p: "+355"
                }

                ListElement {
                    n: "Algeria"
                    i: "DZ"
                    p: "+213"
                }

                ListElement {
                    n: "American Samoa"
                    i: "AS"
                    p: "+1 684"
                }

                ListElement {
                    n: "Andorra"
                    i: "AD"
                    p: "+376"
                }

                ListElement {
                    n: "Angola"
                    i: "AO"
                    p: "+244"
                }

                ListElement {
                    n: "Anguilla"
                    i: "AI"
                    p: "+1 264"
                }

                ListElement {
                    n: "Antigua and Barbuda"
                    i: "AG"
                    p: "+1 268"
                }

                ListElement {
                    n: "Argentina"
                    i: "AR"
                    p: "+54"
                }

                ListElement {
                    n: "Armenia"
                    i: "AM"
                    p: "+374"
                }

                ListElement {
                    n: "Aruba"
                    i: "AW"
                    p: "+297"
                }

                ListElement {
                    n: "Ascension"
                    i: "SH"
                    p: "+247"
                }

                ListElement {
                    n: "Australia"
                    i: "AU"
                    p: "+61"
                }

                ListElement {
                    n: "Australian Antarctic Territory"
                    i: "AQ"
                    p: "+672 1"
                }
                //ListElement{n:"Australian External Territories";i:"";p:"+672"} // NO ISO

                ListElement {
                    n: "Austria"
                    i: "AT"
                    p: "+43"
                }

                ListElement {
                    n: "Azerbaijan"
                    i: "AZ"
                    p: "+994"
                }

                ListElement {
                    n: "Bahamas"
                    i: "BS"
                    p: "+1 242"
                }

                ListElement {
                    n: "Bahrain"
                    i: "BH"
                    p: "+973"
                }

                ListElement {
                    n: "Bangladesh"
                    i: "BD"
                    p: "+880"
                }

                ListElement {
                    n: "Barbados"
                    i: "BB"
                    p: "+1 246"
                }

                ListElement {
                    n: "Barbuda"
                    i: "AG"
                    p: "+1 268"
                }

                ListElement {
                    n: "Belarus"
                    i: "BY"
                    p: "+375"
                }

                ListElement {
                    n: "Belgium"
                    i: "BE"
                    p: "+32"
                }

                ListElement {
                    n: "Belize"
                    i: "BZ"
                    p: "+501"
                }

                ListElement {
                    n: "Benin"
                    i: "BJ"
                    p: "+229"
                }

                ListElement {
                    n: "Bermuda"
                    i: "BM"
                    p: "+1 441"
                }

                ListElement {
                    n: "Bhutan"
                    i: "BT"
                    p: "+975"
                }

                ListElement {
                    n: "Bolivia"
                    i: "BO"
                    p: "+591"
                }

                ListElement {
                    n: "Bonaire"
                    i: "BQ"
                    p: "+599 7"
                }

                ListElement {
                    n: "Bosnia and Herzegovina"
                    i: "BA"
                    p: "+387"
                }

                ListElement {
                    n: "Botswana"
                    i: "BW"
                    p: "+267"
                }

                ListElement {
                    n: "Brazil"
                    i: "BR"
                    p: "+55"
                }

                ListElement {
                    n: "British Indian Ocean Territory"
                    i: "IO"
                    p: "+246"
                }

                ListElement {
                    n: "Brunei Darussalam"
                    i: "BN"
                    p: "+673"
                }

                ListElement {
                    n: "Bulgaria"
                    i: "BG"
                    p: "+359"
                }

                ListElement {
                    n: "Burkina Faso"
                    i: "BF"
                    p: "+226"
                }

                ListElement {
                    n: "Burundi"
                    i: "BI"
                    p: "+257"
                }

                ListElement {
                    n: "Cambodia"
                    i: "KH"
                    p: "+855"
                }

                ListElement {
                    n: "Cameroon"
                    i: "CM"
                    p: "+237"
                }

                ListElement {
                    n: "Canada"
                    i: "CA"
                    p: "+1"
                }

                ListElement {
                    n: "Cape Verde"
                    i: "CV"
                    p: "+238"
                }
                //ListElement{n:"Caribbean Netherlands";i:"";p:"+599 3"} // NO ISO

                //ListElement{n:"Caribbean Netherlands";i:"";p:"+599 4"} // NO ISO
                //ListElement{n:"Caribbean Netherlands";i:"";p:"+599 7"} // NO ISO
                ListElement {
                    n: "Cayman Islands"
                    i: "KY"
                    p: "+1 345"
                }

                ListElement {
                    n: "Central African Republic"
                    i: "CF"
                    p: "+236"
                }

                ListElement {
                    n: "Chad"
                    i: "TD"
                    p: "+235"
                }

                ListElement {
                    n: "Chatham Island (New Zealand)"
                    i: "NZ"
                    p: "+64"
                }

                ListElement {
                    n: "Chile"
                    i: "CL"
                    p: "+56"
                }

                ListElement {
                    n: "China"
                    i: "CN"
                    p: "+86"
                }

                ListElement {
                    n: "Christmas Island"
                    i: "CX"
                    p: "+61 89164"
                }

                ListElement {
                    n: "Cocos (Keeling) Islands"
                    i: "CC"
                    p: "+61 89162"
                }

                ListElement {
                    n: "Colombia"
                    i: "CO"
                    p: "+57"
                }

                ListElement {
                    n: "Comoros"
                    i: "KM"
                    p: "+269"
                }

                ListElement {
                    n: "Congo (Democratic Republic of the)"
                    i: "CD"
                    p: "+243"
                }

                ListElement {
                    n: "Congo"
                    i: "CG"
                    p: "+242"
                }

                ListElement {
                    n: "Cook Islands"
                    i: "CK"
                    p: "+682"
                }

                ListElement {
                    n: "Costa Rica"
                    i: "CR"
                    p: "+506"
                }

                ListElement {
                    n: "Côte d'Ivoire"
                    i: "CI"
                    p: "+225"
                }

                ListElement {
                    n: "Croatia"
                    i: "HR"
                    p: "+385"
                }

                ListElement {
                    n: "Cuba"
                    i: "CU"
                    p: "+53"
                }

                ListElement {
                    n: "Curaçao"
                    i: "CW"
                    p: "+599 9"
                }

                ListElement {
                    n: "Cyprus"
                    i: "CY"
                    p: "+357"
                }

                ListElement {
                    n: "Czech Republic"
                    i: "CZ"
                    p: "+420"
                }

                ListElement {
                    n: "Denmark"
                    i: "DK"
                    p: "+45"
                }
                //ListElement{n:"Diego Garcia";i:"";p:"+246"} // NO ISO, OCC. BY GB

                ListElement {
                    n: "Djibouti"
                    i: "DJ"
                    p: "+253"
                }

                ListElement {
                    n: "Dominica"
                    i: "DM"
                    p: "+1 767"
                }

                ListElement {
                    n: "Dominican Republic"
                    i: "DO"
                    p: "+1 809"
                }

                ListElement {
                    n: "Dominican Republic"
                    i: "DO"
                    p: "+1 829"
                }

                ListElement {
                    n: "Dominican Republic"
                    i: "DO"
                    p: "+1 849"
                }

                ListElement {
                    n: "Easter Island"
                    i: "CL"
                    p: "+56"
                }

                ListElement {
                    n: "Ecuador"
                    i: "EC"
                    p: "+593"
                }

                ListElement {
                    n: "Egypt"
                    i: "EG"
                    p: "+20"
                }

                ListElement {
                    n: "El Salvador"
                    i: "SV"
                    p: "+503"
                }

                ListElement {
                    n: "Equatorial Guinea"
                    i: "GQ"
                    p: "+240"
                }

                ListElement {
                    n: "Eritrea"
                    i: "ER"
                    p: "+291"
                }

                ListElement {
                    n: "Estonia"
                    i: "EE"
                    p: "+372"
                }

                ListElement {
                    n: "eSwatini"
                    i: "SZ"
                    p: "+268"
                }

                ListElement {
                    n: "Ethiopia"
                    i: "ET"
                    p: "+251"
                }

                ListElement {
                    n: "Falkland Islands (Malvinas)"
                    i: "FK"
                    p: "+500"
                }

                ListElement {
                    n: "Faroe Islands"
                    i: "FO"
                    p: "+298"
                }

                ListElement {
                    n: "Fiji"
                    i: "FJ"
                    p: "+679"
                }

                ListElement {
                    n: "Finland"
                    i: "FI"
                    p: "+358"
                }

                ListElement {
                    n: "France"
                    i: "FR"
                    p: "+33"
                }
                //ListElement{n:"French Antilles";i:"";p:"+596"} // NO ISO

                ListElement {
                    n: "French Guiana"
                    i: "GF"
                    p: "+594"
                }

                ListElement {
                    n: "French Polynesia"
                    i: "PF"
                    p: "+689"
                }

                ListElement {
                    n: "Gabon"
                    i: "GA"
                    p: "+241"
                }

                ListElement {
                    n: "Gambia"
                    i: "GM"
                    p: "+220"
                }

                ListElement {
                    n: "Georgia"
                    i: "GE"
                    p: "+995"
                }

                ListElement {
                    n: "Germany"
                    i: "DE"
                    p: "+49"
                }

                ListElement {
                    n: "Ghana"
                    i: "GH"
                    p: "+233"
                }

                ListElement {
                    n: "Gibraltar"
                    i: "GI"
                    p: "+350"
                }

                ListElement {
                    n: "Greece"
                    i: "GR"
                    p: "+30"
                }

                ListElement {
                    n: "Greenland"
                    i: "GL"
                    p: "+299"
                }

                ListElement {
                    n: "Grenada"
                    i: "GD"
                    p: "+1 473"
                }

                ListElement {
                    n: "Guadeloupe"
                    i: "GP"
                    p: "+590"
                }

                ListElement {
                    n: "Guam"
                    i: "GU"
                    p: "+1 671"
                }

                ListElement {
                    n: "Guatemala"
                    i: "GT"
                    p: "+502"
                }

                ListElement {
                    n: "Guernsey"
                    i: "GG"
                    p: "+44 1481"
                }

                ListElement {
                    n: "Guernsey"
                    i: "GG"
                    p: "+44 7781"
                }

                ListElement {
                    n: "Guernsey"
                    i: "GG"
                    p: "+44 7839"
                }

                ListElement {
                    n: "Guernsey"
                    i: "GG"
                    p: "+44 7911"
                }

                ListElement {
                    n: "Guinea-Bissau"
                    i: "GW"
                    p: "+245"
                }

                ListElement {
                    n: "Guinea"
                    i: "GN"
                    p: "+224"
                }

                ListElement {
                    n: "Guyana"
                    i: "GY"
                    p: "+592"
                }

                ListElement {
                    n: "Haiti"
                    i: "HT"
                    p: "+509"
                }

                ListElement {
                    n: "Honduras"
                    i: "HN"
                    p: "+504"
                }

                ListElement {
                    n: "Hong Kong"
                    i: "HK"
                    p: "+852"
                }

                ListElement {
                    n: "Hungary"
                    i: "HU"
                    p: "+36"
                }

                ListElement {
                    n: "Iceland"
                    i: "IS"
                    p: "+354"
                }

                ListElement {
                    n: "India"
                    i: "IN"
                    p: "+91"
                }

                ListElement {
                    n: "Indonesia"
                    i: "ID"
                    p: "+62"
                }

                ListElement {
                    n: "Iran"
                    i: "IR"
                    p: "+98"
                }

                ListElement {
                    n: "Iraq"
                    i: "IQ"
                    p: "+964"
                }

                ListElement {
                    n: "Ireland"
                    i: "IE"
                    p: "+353"
                }

                ListElement {
                    n: "Isle of Man"
                    i: "IM"
                    p: "+44 1624"
                }

                ListElement {
                    n: "Isle of Man"
                    i: "IM"
                    p: "+44 7524"
                }

                ListElement {
                    n: "Isle of Man"
                    i: "IM"
                    p: "+44 7624"
                }

                ListElement {
                    n: "Isle of Man"
                    i: "IM"
                    p: "+44 7924"
                }

                ListElement {
                    n: "Israel"
                    i: "IL"
                    p: "+972"
                }

                ListElement {
                    n: "Italy"
                    i: "IT"
                    p: "+39"
                }

                ListElement {
                    n: "Jamaica"
                    i: "JM"
                    p: "+1 876"
                }

                ListElement {
                    n: "Jan Mayen"
                    i: "SJ"
                    p: "+47 79"
                }

                ListElement {
                    n: "Japan"
                    i: "JP"
                    p: "+81"
                }

                ListElement {
                    n: "Jersey"
                    i: "JE"
                    p: "+44 1534"
                }

                ListElement {
                    n: "Jordan"
                    i: "JO"
                    p: "+962"
                }

                ListElement {
                    n: "Kazakhstan"
                    i: "KZ"
                    p: "+7 6"
                }

                ListElement {
                    n: "Kazakhstan"
                    i: "KZ"
                    p: "+7 7"
                }

                ListElement {
                    n: "Kenya"
                    i: "KE"
                    p: "+254"
                }

                ListElement {
                    n: "Kiribati"
                    i: "KI"
                    p: "+686"
                }

                ListElement {
                    n: "Korea (North)"
                    i: "KP"
                    p: "+850"
                }

                ListElement {
                    n: "Korea (South)"
                    i: "KR"
                    p: "+82"
                }
                // TEMP. CODE

                ListElement {
                    n: "Kosovo"
                    i: "XK"
                    p: "+383"
                }

                ListElement {
                    n: "Kuwait"
                    i: "KW"
                    p: "+965"
                }

                ListElement {
                    n: "Kyrgyzstan"
                    i: "KG"
                    p: "+996"
                }

                ListElement {
                    n: "Laos"
                    i: "LA"
                    p: "+856"
                }

                ListElement {
                    n: "Latvia"
                    i: "LV"
                    p: "+371"
                }

                ListElement {
                    n: "Lebanon"
                    i: "LB"
                    p: "+961"
                }

                ListElement {
                    n: "Lesotho"
                    i: "LS"
                    p: "+266"
                }

                ListElement {
                    n: "Liberia"
                    i: "LR"
                    p: "+231"
                }

                ListElement {
                    n: "Libya"
                    i: "LY"
                    p: "+218"
                }

                ListElement {
                    n: "Liechtenstein"
                    i: "LI"
                    p: "+423"
                }

                ListElement {
                    n: "Lithuania"
                    i: "LT"
                    p: "+370"
                }

                ListElement {
                    n: "Luxembourg"
                    i: "LU"
                    p: "+352"
                }

                ListElement {
                    n: "Macau (Macao)"
                    i: "MO"
                    p: "+853"
                }

                ListElement {
                    n: "Madagascar"
                    i: "MG"
                    p: "+261"
                }

                ListElement {
                    n: "Malawi"
                    i: "MW"
                    p: "+265"
                }

                ListElement {
                    n: "Malaysia"
                    i: "MY"
                    p: "+60"
                }

                ListElement {
                    n: "Maldives"
                    i: "MV"
                    p: "+960"
                }

                ListElement {
                    n: "Mali"
                    i: "ML"
                    p: "+223"
                }

                ListElement {
                    n: "Malta"
                    i: "MT"
                    p: "+356"
                }

                ListElement {
                    n: "Marshall Islands"
                    i: "MH"
                    p: "+692"
                }

                ListElement {
                    n: "Martinique"
                    i: "MQ"
                    p: "+596"
                }

                ListElement {
                    n: "Mauritania"
                    i: "MR"
                    p: "+222"
                }

                ListElement {
                    n: "Mauritius"
                    i: "MU"
                    p: "+230"
                }

                ListElement {
                    n: "Mayotte"
                    i: "YT"
                    p: "+262 269"
                }

                ListElement {
                    n: "Mayotte"
                    i: "YT"
                    p: "+262 639"
                }

                ListElement {
                    n: "Mexico"
                    i: "MX"
                    p: "+52"
                }

                ListElement {
                    n: "Micronesia (Federated States of)"
                    i: "FM"
                    p: "+691"
                }

                ListElement {
                    n: "Midway Island (USA)"
                    i: "US"
                    p: "+1 808"
                }

                ListElement {
                    n: "Moldova"
                    i: "MD"
                    p: "+373"
                }

                ListElement {
                    n: "Monaco"
                    i: "MC"
                    p: "+377"
                }

                ListElement {
                    n: "Mongolia"
                    i: "MN"
                    p: "+976"
                }

                ListElement {
                    n: "Montenegro"
                    i: "ME"
                    p: "+382"
                }

                ListElement {
                    n: "Montserrat"
                    i: "MS"
                    p: "+1 664"
                }

                ListElement {
                    n: "Morocco"
                    i: "MA"
                    p: "+212"
                }

                ListElement {
                    n: "Mozambique"
                    i: "MZ"
                    p: "+258"
                }

                ListElement {
                    n: "Myanmar"
                    i: "MM"
                    p: "+95"
                }
                // NO OWN ISO, DISPUTED

                ListElement {
                    n: "Nagorno-Karabakh"
                    i: "AZ"
                    p: "+374 47"
                }
                // NO OWN ISO, DISPUTED

                ListElement {
                    n: "Nagorno-Karabakh"
                    i: "AZ"
                    p: "+374 97"
                }

                ListElement {
                    n: "Namibia"
                    i: "NA"
                    p: "+264"
                }

                ListElement {
                    n: "Nauru"
                    i: "NR"
                    p: "+674"
                }

                ListElement {
                    n: "Nepal"
                    i: "NP"
                    p: "+977"
                }

                ListElement {
                    n: "Netherlands"
                    i: "NL"
                    p: "+31"
                }

                ListElement {
                    n: "Nevis"
                    i: "KN"
                    p: "+1 869"
                }

                ListElement {
                    n: "New Caledonia"
                    i: "NC"
                    p: "+687"
                }

                ListElement {
                    n: "New Zealand"
                    i: "NZ"
                    p: "+64"
                }

                ListElement {
                    n: "Nicaragua"
                    i: "NI"
                    p: "+505"
                }

                ListElement {
                    n: "Nigeria"
                    i: "NG"
                    p: "+234"
                }

                ListElement {
                    n: "Niger"
                    i: "NE"
                    p: "+227"
                }

                ListElement {
                    n: "Niue"
                    i: "NU"
                    p: "+683"
                }

                ListElement {
                    n: "Norfolk Island"
                    i: "NF"
                    p: "+672 3"
                }
                // OCC. BY TR

                ListElement {
                    n: "Northern Cyprus"
                    i: "CY"
                    p: "+90 392"
                }

                ListElement {
                    n: "Northern Ireland"
                    i: "GB"
                    p: "+44 28"
                }

                ListElement {
                    n: "Northern Mariana Islands"
                    i: "MP"
                    p: "+1 670"
                }

                ListElement {
                    n: "North Macedonia"
                    i: "MK"
                    p: "+389"
                }

                ListElement {
                    n: "Norway"
                    i: "NO"
                    p: "+47"
                }

                ListElement {
                    n: "Oman"
                    i: "OM"
                    p: "+968"
                }

                ListElement {
                    n: "Pakistan"
                    i: "PK"
                    p: "+92"
                }

                ListElement {
                    n: "Palau"
                    i: "PW"
                    p: "+680"
                }

                ListElement {
                    n: "Palestine (State of)"
                    i: "PS"
                    p: "+970"
                }

                ListElement {
                    n: "Panama"
                    i: "PA"
                    p: "+507"
                }

                ListElement {
                    n: "Papua New Guinea"
                    i: "PG"
                    p: "+675"
                }

                ListElement {
                    n: "Paraguay"
                    i: "PY"
                    p: "+595"
                }

                ListElement {
                    n: "Peru"
                    i: "PE"
                    p: "+51"
                }

                ListElement {
                    n: "Philippines"
                    i: "PH"
                    p: "+63"
                }

                ListElement {
                    n: "Pitcairn Islands"
                    i: "PN"
                    p: "+64"
                }

                ListElement {
                    n: "Poland"
                    i: "PL"
                    p: "+48"
                }

                ListElement {
                    n: "Portugal"
                    i: "PT"
                    p: "+351"
                }

                ListElement {
                    n: "Puerto Rico"
                    i: "PR"
                    p: "+1 787"
                }

                ListElement {
                    n: "Puerto Rico"
                    i: "PR"
                    p: "+1 939"
                }

                ListElement {
                    n: "Qatar"
                    i: "QA"
                    p: "+974"
                }

                ListElement {
                    n: "Réunion"
                    i: "RE"
                    p: "+262"
                }

                ListElement {
                    n: "Romania"
                    i: "RO"
                    p: "+40"
                }

                ListElement {
                    n: "Russia"
                    i: "RU"
                    p: "+7"
                }

                ListElement {
                    n: "Rwanda"
                    i: "RW"
                    p: "+250"
                }

                ListElement {
                    n: "Saba"
                    i: "BQ"
                    p: "+599 4"
                }

                ListElement {
                    n: "Saint Barthélemy"
                    i: "BL"
                    p: "+590"
                }

                ListElement {
                    n: "Saint Helena"
                    i: "SH"
                    p: "+290"
                }

                ListElement {
                    n: "Saint Kitts and Nevis"
                    i: "KN"
                    p: "+1 869"
                }

                ListElement {
                    n: "Saint Lucia"
                    i: "LC"
                    p: "+1 758"
                }

                ListElement {
                    n: "Saint Martin (France)"
                    i: "MF"
                    p: "+590"
                }

                ListElement {
                    n: "Saint Pierre and Miquelon"
                    i: "PM"
                    p: "+508"
                }

                ListElement {
                    n: "Saint Vincent and the Grenadines"
                    i: "VC"
                    p: "+1 784"
                }

                ListElement {
                    n: "Samoa"
                    i: "WS"
                    p: "+685"
                }

                ListElement {
                    n: "San Marino"
                    i: "SM"
                    p: "+378"
                }

                ListElement {
                    n: "São Tomé and Príncipe"
                    i: "ST"
                    p: "+239"
                }

                ListElement {
                    n: "Saudi Arabia"
                    i: "SA"
                    p: "+966"
                }

                ListElement {
                    n: "Senegal"
                    i: "SN"
                    p: "+221"
                }

                ListElement {
                    n: "Serbia"
                    i: "RS"
                    p: "+381"
                }

                ListElement {
                    n: "Seychelles"
                    i: "SC"
                    p: "+248"
                }

                ListElement {
                    n: "Sierra Leone"
                    i: "SL"
                    p: "+232"
                }

                ListElement {
                    n: "Singapore"
                    i: "SG"
                    p: "+65"
                }

                ListElement {
                    n: "Sint Eustatius"
                    i: "BQ"
                    p: "+599 3"
                }

                ListElement {
                    n: "Sint Maarten (Netherlands)"
                    i: "SX"
                    p: "+1 721"
                }

                ListElement {
                    n: "Slovakia"
                    i: "SK"
                    p: "+421"
                }

                ListElement {
                    n: "Slovenia"
                    i: "SI"
                    p: "+386"
                }

                ListElement {
                    n: "Solomon Islands"
                    i: "SB"
                    p: "+677"
                }

                ListElement {
                    n: "Somalia"
                    i: "SO"
                    p: "+252"
                }

                ListElement {
                    n: "South Africa"
                    i: "ZA"
                    p: "+27"
                }

                ListElement {
                    n: "South Georgia and the South Sandwich Islands"
                    i: "GS"
                    p: "+500"
                }
                // NO OWN ISO, DISPUTED

                ListElement {
                    n: "South Ossetia"
                    i: "GE"
                    p: "+995 34"
                }

                ListElement {
                    n: "South Sudan"
                    i: "SS"
                    p: "+211"
                }

                ListElement {
                    n: "Spain"
                    i: "ES"
                    p: "+34"
                }

                ListElement {
                    n: "Sri Lanka"
                    i: "LK"
                    p: "+94"
                }

                ListElement {
                    n: "Sudan"
                    i: "SD"
                    p: "+249"
                }

                ListElement {
                    n: "Suriname"
                    i: "SR"
                    p: "+597"
                }

                ListElement {
                    n: "Svalbard"
                    i: "SJ"
                    p: "+47 79"
                }

                ListElement {
                    n: "Sweden"
                    i: "SE"
                    p: "+46"
                }

                ListElement {
                    n: "Switzerland"
                    i: "CH"
                    p: "+41"
                }

                ListElement {
                    n: "Syria"
                    i: "SY"
                    p: "+963"
                }

                ListElement {
                    n: "Taiwan"
                    i: "SJ"
                    p: "+886"
                }

                ListElement {
                    n: "Tajikistan"
                    i: "TJ"
                    p: "+992"
                }

                ListElement {
                    n: "Tanzania"
                    i: "TZ"
                    p: "+255"
                }

                ListElement {
                    n: "Thailand"
                    i: "TH"
                    p: "+66"
                }

                ListElement {
                    n: "Timor-Leste"
                    i: "TL"
                    p: "+670"
                }

                ListElement {
                    n: "Togo"
                    i: "TG"
                    p: "+228"
                }

                ListElement {
                    n: "Tokelau"
                    i: "TK"
                    p: "+690"
                }

                ListElement {
                    n: "Tonga"
                    i: "TO"
                    p: "+676"
                }

                ListElement {
                    n: "Transnistria"
                    i: "MD"
                    p: "+373 2"
                }

                ListElement {
                    n: "Transnistria"
                    i: "MD"
                    p: "+373 5"
                }

                ListElement {
                    n: "Trinidad and Tobago"
                    i: "TT"
                    p: "+1 868"
                }

                ListElement {
                    n: "Tristan da Cunha"
                    i: "SH"
                    p: "+290 8"
                }

                ListElement {
                    n: "Tunisia"
                    i: "TN"
                    p: "+216"
                }

                ListElement {
                    n: "Turkey"
                    i: "TR"
                    p: "+90"
                }

                ListElement {
                    n: "Turkmenistan"
                    i: "TM"
                    p: "+993"
                }

                ListElement {
                    n: "Turks and Caicos Islands"
                    i: "TC"
                    p: "+1 649"
                }

                ListElement {
                    n: "Tuvalu"
                    i: "TV"
                    p: "+688"
                }

                ListElement {
                    n: "Uganda"
                    i: "UG"
                    p: "+256"
                }

                ListElement {
                    n: "Ukraine"
                    i: "UA"
                    p: "+380"
                }

                ListElement {
                    n: "United Arab Emirates"
                    i: "AE"
                    p: "+971"
                }

                ListElement {
                    n: "United Kingdom"
                    i: "GB"
                    p: "+44"
                }

                ListElement {
                    n: "United States"
                    i: "US"
                    p: "+1"
                }

                ListElement {
                    n: "Uruguay"
                    i: "UY"
                    p: "+598"
                }

                ListElement {
                    n: "Uzbekistan"
                    i: "UZ"
                    p: "+998"
                }

                ListElement {
                    n: "Vanuatu"
                    i: "VU"
                    p: "+678"
                }

                ListElement {
                    n: "Vatican City State (Holy See)"
                    i: "VA"
                    p: "+379"
                }

                ListElement {
                    n: "Vatican City State (Holy See)"
                    i: "VA"
                    p: "+39 06 698"
                }

                ListElement {
                    n: "Venezuela"
                    i: "VE"
                    p: "+58"
                }

                ListElement {
                    n: "Vietnam"
                    i: "VN"
                    p: "+84"
                }

                ListElement {
                    n: "Virgin Islands (British)"
                    i: "VG"
                    p: "+1 284"
                }

                ListElement {
                    n: "Virgin Islands (US)"
                    i: "VI"
                    p: "+1 340"
                }

                ListElement {
                    n: "Wake Island (USA)"
                    i: "US"
                    p: "+1 808"
                }

                ListElement {
                    n: "Wallis and Futuna"
                    i: "WF"
                    p: "+681"
                }

                ListElement {
                    n: "Yemen"
                    i: "YE"
                    p: "+967"
                }

                ListElement {
                    n: "Zambia"
                    i: "ZM"
                    p: "+260"
                }
                // NO OWN ISO, DISPUTED?

                ListElement {
                    n: "Zanzibar"
                    i: "TZ"
                    p: "+255 24"
                }

                ListElement {
                    n: "Zimbabwe"
                    i: "ZW"
                    p: "+263"
                }

            }

        }

        MatrixTextField {
            id: statusInput

            Layout.fillWidth: true
        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            if (inputDialog.onAccepted)
                inputDialog.onAccepted(numberPrefix.model.get(numberPrefix.currentIndex).i, statusInput.text);

            inputDialog.close();
        }
        onRejected: {
            inputDialog.close();
        }
    }

}
