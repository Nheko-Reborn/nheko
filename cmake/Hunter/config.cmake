hunter_config(
    Boost
    VERSION  "1.70.0-p1"
    CMAKE_ARGS IOSTREAMS_NO_BZIP2=1
)
hunter_config(
    nlohmann_json
    CMAKE_ARGS JSON_MultipleHeaders=ON
)
