run: build
	@./build/nheko

build:
	@cmake -H. -Bbuild
	@make -C build

clean:
	rm -rf build

.PHONY: build
