SRC := $(shell find include src -type f -type f \( -iname "*.cc" -o -iname "*.h" \))

# Linux specific helpers
debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build


release-debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

test:
	@cmake -DBUILD_TESTS=ON -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build
	@cd build && GTEST_COLOR=1 ctest --verbose

# MacOS specific helpers
mdebug:
	@cmake -DBUILD_TESTS=OFF \
		-H. \
		-GNinja \
		-Bbuild \
		-DCMAKE_PREFIX_PATH=/usr/local/opt/qt5 \
		-DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

mrelease:
	@cmake -DBUILD_TESTS=OFF \
		-H. \
		-GNinja \
		-Bbuild \
		-DCMAKE_PREFIX_PATH=/usr/local/opt/qt5 \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

run:
	@./build/nheko

lint:
	@clang-format -i $(SRC)


clean:
	rm -rf build

.PHONY: build
