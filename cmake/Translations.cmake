#
# Generate the translation resource file
#

FILE(GLOB LANG_TS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/langs/*.ts")

qt5_add_translation(QM_SRC ${LANG_TS_SRC})
add_custom_target(LANG_QRC ALL DEPENDS ${QM_SRC})

# Generate a qrc file for the translations
set(_qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)

if(NOT EXISTS ${_qrc})
    file(WRITE ${_qrc} "<RCC> <qresource prefix=\"/translations\">")
    foreach(_lang ${QM_SRC})
        get_filename_component(_filename ${_lang} NAME)
        file(APPEND ${_qrc} "<file>${_filename}</file>")
    endforeach(_lang)
    file(APPEND ${_qrc} "</qresource> </RCC>")
endif()

qt5_add_resources(LANG_QRC ${_qrc})
qt5_add_resources(QRC resources/res.qrc)
