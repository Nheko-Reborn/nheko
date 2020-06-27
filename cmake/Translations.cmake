#
# Generate the translation resource file
#

file(GLOB LANG_TS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/langs/*.ts")

qt5_add_translation(QM_SRC ${LANG_TS_SRC})
qt5_create_translation(${QM_SRC})
add_custom_target(LANG_QRC ALL DEPENDS ${QM_SRC})

# Generate a qrc file for the translations
set(_qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)

if(NOT EXISTS ${_qrc})
  file(WRITE ${_qrc} "<RCC>\n <qresource prefix=\"/translations\">\n")
  foreach(_lang ${QM_SRC})
    get_filename_component(_filename ${_lang} NAME)
    file(APPEND ${_qrc} "  <file>${_filename}</file>\n")
  endforeach(_lang)
  file(APPEND ${_qrc} " </qresource>\n</RCC>\n")
endif()

qt5_add_resources(LANG_QRC ${_qrc})
if(Qt5QuickCompiler_FOUND AND COMPILE_QML)
	qtquick_compiler_add_resources(QRC resources/res.qrc)
else()
	qt5_add_resources(QRC resources/res.qrc)
endif()
