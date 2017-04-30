SRC := $(shell find include src -type f -type f \( -iname "*.cc" -o -iname "*.h" \))

debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

run:
	@./build/nheko

lint:
	@clang-format -i $(SRC)

clean:
	rm -rf build

.PHONY: build
