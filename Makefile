run: build
	@./build/nheko

build:
	@cmake -H. -Bbuild
	@make -C build

release:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
	@make -C build

clean:
	rm -rf build

.PHONY: build
