#include "blurhash.hpp"

#include <charconv>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int
main(int argc, char **argv)
{
        if (argc != 5) {
                std::cerr << "Usage: blurhash2bmp [hash] [width] [height] [output name]"
                          << std::endl;
                return -2;
        }

        int height = 0, width = 0;

        std::string_view width_str{argv[2]}, height_str{argv[3]};
        std::from_chars(height_str.begin(), height_str.end(), height);
        if (height <= 0) {
                std::cerr << "Invalid height.";
                return -2;
        }
        std::from_chars(width_str.begin(), width_str.end(), width);
        if (width <= 0) {
                std::cerr << "Invalid width.";
                return -2;
        }

        blurhash::Image image = blurhash::decode(argv[1], width, height);
        if (image.image.empty()) {
                std::cerr << "Decode failed.";
                return -1;
        }
        if (!stbi_write_bmp(argv[4], image.width, image.height, 3, (void *)image.image.data())) {
                std::cerr << "Image write failed.";
                return -1;
        }

        return 0;
}
