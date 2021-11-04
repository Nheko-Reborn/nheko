// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtMultimedia 5.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import im.nheko 1.0

ColumnLayout {
    required property double proportionalHeight
    required property int type
    required property int originalWidth
    required property string thumbnailUrl
    required property string eventId
    required property string url
    required property string body
    required property string filesize

    function durationToString(duration) {
		function maybeZeroPrepend(time) {
			return (time < 10) ? "0" + time.toString() :
				time.toString()
		}
		var totalSeconds = Math.floor(duration / 1000)
		var seconds = totalSeconds % 60
		var minutes = (Math.floor(totalSeconds / 60)) % 60
		var hours = (Math.floor(totalSeconds / (60 * 24))) % 24
		// Always show minutes and don't prepend zero into the leftmost element
		var ss = maybeZeroPrepend(seconds)
		var mm = (hours > 0) ? maybeZeroPrepend(minutes) : minutes.toString()
		var hh = hours.toString()

		if (hours < 1)
			return mm + ":" + ss
		return hh + ":" + mm + ":" + ss
	}

    id: content
    Layout.maximumWidth: parent? parent.width: undefined
    MxcMedia {
        id: mxcmedia
        // TODO: Show error in overlay or so?
        onError: console.log(error)
        roomm: room
		onMediaStatusChanged: {
			if (status == MxcMedia.LoadedMedia) {
				progress.updatePositionTexts();
			}
		}
    }
      
    Rectangle {
        id: videoContainer
        visible: type == MtxEvent.VideoMessage
        //property double tempWidth: Math.min(parent ? parent.width : undefined, model.data.width < 1 ? 400 : /////model.data.width)
        // property double tempWidth: (model.data.width < 1) ? 400 : model.data.width
        // property double tempHeight: tempWidth * model.data.proportionalHeight
        //property double tempWidth: Math.min(parent ? parent.width : undefined, originalWidth < 1 ? 400 : originalWidth)
        property double tempWidth: Math.min(parent ? parent.width: undefined, originalWidth < 1 ? 400 : originalWidth)
        property double tempHeight: tempWidth * proportionalHeight

        property double divisor: isReply ? 4 : 2
        property bool tooHigh: tempHeight > timelineRoot.height / divisor

        Layout.maximumWidth: Layout.preferredWidth
        Layout.preferredHeight: tooHigh ? timelineRoot.height / divisor : tempHeight
        Layout.preferredWidth: tooHigh ? (timelineRoot.height / divisor) / proportionalHeight : tempWidth
        Image {
            anchors.fill: parent
            source: thumbnailUrl.replace("mxc://", "image://MxcImage/")
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            // Button and window colored overlay to cache media
            Rectangle {
                // Display over video controls
                z: videoOutput.z + 1
                visible: !mxcmedia.loaded
                anchors.fill: parent
                color: Nheko.colors.window
                opacity: 0.5
                Image {
                    property color buttonColor: (cacheVideoArea.containsMouse) ? Nheko.colors.highlight :
                        Nheko.colors.text

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "image://colorimage/:/icons/icons/ui/arrow-pointing-down.png?"+buttonColor						
                }
                MouseArea {
                    id: cacheVideoArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: !mxcmedia.loaded
                    onClicked: mxcmedia.eventId = eventId
                }
            }
            VideoOutput {
                id: videoOutput
                clip: true
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                source: mxcmedia
				flushMode: VideoOutput.FirstFrame

                // TODO: once we can use Qt 5.12, use HoverHandler
                MouseArea {
                    id: playerMouseArea
                    // Toggle play state on clicks
                    onClicked: {
                        if (controlRect.shouldShowControls &&
                            !controlRect.contains(mapToItem(controlRect, mouseX, mouseY))) {
                                (mxcmedia.state == MediaPlayer.PlayingState) ?
                                    mxcmedia.pause() :
                                    mxcmedia.play()
                        }
                    }
					Rectangle {
						id: controlRect
						property int controlHeight: 25
						property bool shouldShowControls: playerMouseArea.shouldShowControls ||
							volumeSliderRect.visible

						anchors.bottom: playerMouseArea.bottom
						// Window color with 128/255 alpha
						color: {
							var wc = Nheko.colors.window
							return Qt.rgba(wc.r, wc.g, wc.b, 0.5)
						}
						height: 40
						width: playerMouseArea.width
						opacity: shouldShowControls ? 1 : 0
						// Fade controls in/out
						Behavior on opacity {
							OpacityAnimator {
								duration: 100
							}
						}

						RowLayout {
							anchors.fill: parent
							width: parent.width
							// Play/pause button
							Image {
								id: playbackStateImage
								fillMode: Image.PreserveAspectFit
								Layout.preferredHeight: controlRect.controlHeight
								Layout.alignment: Qt.AlignVCenter
								property color controlColor: (playbackStateArea.containsMouse) ?
									Nheko.colors.highlight : Nheko.colors.text

								source: (mxcmedia.state == MediaPlayer.PlayingState) ?
									"image://colorimage/:/icons/icons/ui/pause-symbol.png?"+controlColor :
									"image://colorimage/:/icons/icons/ui/play-sign.png?"+controlColor
								MouseArea {
									id: playbackStateArea
									
									anchors.fill: parent
									hoverEnabled: true
									onClicked: {
										(mxcmedia.state == MediaPlayer.PlayingState) ?
											mxcmedia.pause() :
											mxcmedia.play()
									}
								}
							}
							Label {
								text: (!mxcmedia.loaded) ? "-/-" :
									durationToString(mxcmedia.position) + "/" + durationToString(mxcmedia.duration)
							}

							Slider {
								Layout.fillWidth: true
								Layout.minimumWidth: 50
								height: controlRect.controlHeight
								value: mxcmedia.position
								onMoved: mxcmedia.position = value
								from: 0
								to: mxcmedia.duration
							}
							// Volume slider activator
							Image {
								property color controlColor: (volumeImageArea.containsMouse) ?
									Nheko.colors.highlight : Nheko.colors.text

								// TODO: add icons for different volume levels
								id: volumeImage
								source: (mxcmedia.volume > 0 && !mxcmedia.muted) ?
									"image://colorimage/:/icons/icons/ui/volume-up.png?"+ controlColor :
									"image://colorimage/:/icons/icons/ui/volume-off-indicator.png?"+ controlColor
								Layout.rightMargin: 5
								Layout.preferredHeight: controlRect.controlHeight
								fillMode: Image.PreserveAspectFit
								MouseArea {
									id: volumeImageArea	
									anchors.fill: parent
									hoverEnabled: true
									onClicked: mxcmedia.muted = !mxcmedia.muted
									onExited: volumeSliderHideTimer.start()
									onPositionChanged: volumeSliderHideTimer.start()
									// For hiding volume slider after a while
									Timer {
										id: volumeSliderHideTimer
										interval: 1500
										repeat: false
										running: false
									}
								}
								Rectangle {
									id: volumeSliderRect
									opacity: (visible) ? 1 : 0
									Behavior on opacity {
										OpacityAnimator {
											duration: 100
										}
									}
									// TODO: figure out a better way to put the slider popup above controlRect
									anchors.bottom: volumeImage.top
									anchors.bottomMargin: 10
									anchors.horizontalCenter: volumeImage.horizontalCenter
									color: {
										var wc = Nheko.colors.window
										return Qt.rgba(wc.r, wc.g, wc.b, 0.5)
									}
									/* TODO: base width on the slider width (some issue with it not having a geometry
										when using the width here?) */
									width: volumeImage.width * 0.7
									radius: volumeSlider.width / 2
									height: controlRect.height * 2 //100
									visible: volumeImageArea.containsMouse ||
										volumeSliderHideTimer.running ||
										volumeSliderRectMouseArea.containsMouse
									Slider {
										// Desired value to avoid loop onMoved -> media.volume -> value -> onMoved...
										property real desiredVolume: 1
										
										// TODO: the slider is slightly off-center on the left for some reason...
										id: volumeSlider
										from: 0
										to: 1
										value: (mxcmedia.muted) ? 0 :
											QtMultimedia.convertVolume(desiredVolume,
												QtMultimedia.LinearVolumeScale,
												QtMultimedia.LogarithmicVolumeScale)
										anchors.fill: parent
										anchors.bottomMargin: parent.height * 0.1
										anchors.topMargin: parent.height * 0.1
										anchors.horizontalCenter: parent.horizontalCenter
										orientation: Qt.Vertical
										onMoved: desiredVolume = QtMultimedia.convertVolume(value,
											QtMultimedia.LogarithmicVolumeScale,
											QtMultimedia.LinearVolumeScale)
										/* This would be better handled in 'media', but it has some issue with listening
											to this signal */
										onDesiredVolumeChanged: mxcmedia.muted = !(desiredVolume > 0)
									}
									// Used for resetting the timer on mouse moves on volumeSliderRect
									MouseArea {
										id: volumeSliderRectMouseArea
										anchors.fill: parent
										hoverEnabled: true
										propagateComposedEvents: true
										onExited: volumeSliderHideTimer.start()

										onClicked: mouse.accepted = false
										onPressed: mouse.accepted = false
										onReleased: mouse.accepted = false
										onPressAndHold: mouse.accepted = false
										onPositionChanged: {
											mouse.accepted = false
											volumeSliderHideTimer.start()
										}
									}
								}
							}

						}
					}
                    // This breaks separation of concerns but this same thing doesn't work when called from controlRect...
                    property bool shouldShowControls: (containsMouse && controlHideTimer.running) ||
                        (mxcmedia.state != MediaPlayer.PlayingState) ||
                        controlRect.contains(mapToItem(controlRect, mouseX, mouseY))

                    // For hiding controls on stationary cursor
                    Timer {
                        id: controlHideTimer
                        interval: 1500 //ms
                        repeat: false
                    }

                    hoverEnabled: true
                    onPositionChanged: controlHideTimer.start()

                    x: videoOutput.contentRect.x
                    y: videoOutput.contentRect.y
                    width: videoOutput.contentRect.width
                    height: videoOutput.contentRect.height
                    propagateComposedEvents: true
                }
            }
        }
    }
    // Audio player
    // TODO: share code with the video player
    Rectangle {
		id: audioControlRect
		
		visible: type != MtxEvent.VideoMessage
		property int controlHeight: 25
		Layout.preferredHeight: 40
		RowLayout {
			anchors.fill: parent
			width: parent.width
			// Play/pause button
			Image {
				id: audioPlaybackStateImage
				fillMode: Image.PreserveAspectFit
				Layout.preferredHeight: controlRect.controlHeight
				Layout.alignment: Qt.AlignVCenter
				property color controlColor: (audioPlaybackStateArea.containsMouse) ?
					Nheko.colors.highlight : Nheko.colors.text

				source: {
                    if (!mxcmedia.loaded)
                        return "image://colorimage/:/icons/icons/ui/arrow-pointing-down.png?"+controlColor
                    return (mxcmedia.state == MediaPlayer.PlayingState) ?
                        "image://colorimage/:/icons/icons/ui/pause-symbol.png?"+controlColor :
                        "image://colorimage/:/icons/icons/ui/play-sign.png?"+controlColor
                }
				MouseArea {
					id: audioPlaybackStateArea
					
					anchors.fill: parent
					hoverEnabled: true
					onClicked: {
                        if (!mxcmedia.loaded) {
                            mxcmedia.eventId = eventId
                            return
                        }
						(mxcmedia.state == MediaPlayer.PlayingState) ?
							mxcmedia.pause() :
							mxcmedia.play()
					}
				}
			}
			Label {
				text: (!mxcmedia.loaded) ? "-/-" :
					durationToString(mxcmedia.position) + "/" + durationToString(mxcmedia.duration)
			}

			Slider {
				Layout.fillWidth: true
				Layout.minimumWidth: 50
				height: controlRect.controlHeight
				value: mxcmedia.position
				onMoved: mxcmedia.seek(value)
				from: 0
				to: mxcmedia.duration
			}
		}
	}
    
    Label {
        id: fileInfoLabel
        
        background: Rectangle {
            color: Nheko.colors.base
        }
		Layout.fillWidth: true
		text: body + " [" + filesize + "]"
		textFormat: Text.PlainText
		elide: Text.ElideRight
		color: Nheko.colors.text
	}
}
