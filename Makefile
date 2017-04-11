debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

run:
	@./build/nheko

clean:
	rm -rf build

.PHONY: build
