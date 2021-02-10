TextMessage {
    font.italic: true
    color: colors.buttonText
    height: isReply ? Math.min(timelineRoot.height / 8, implicitHeight) : undefined
    clip: isReply
}
