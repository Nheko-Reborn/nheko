SRC := $(shell find include src -type f -type f \( -iname "*.cc" -o -iname "*.h" \))

debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

run:
	@./build/nheko

lint:
	@clang-format -i $(SRC)

test:
	@cmake -DBUILD_TESTS=ON -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Release
	@cmake --build build
	@cd build && GTEST_COLOR=1 ctest --verbose

clean:
	rm -rf build

.PHONY: build
