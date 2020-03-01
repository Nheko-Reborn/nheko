#include "blurhash.hpp"

#include <charconv>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int
main(int argc, char **argv)
{
        if (argc != 4) {
                std::cerr << "Usage: blurhash [filename] [num x components] [num y components]"
                          << std::endl;
                return -2;
        }

        int x = 0, y = 0;

        std::string_view x_str{argv[2]}, y_str{argv[3]};
        std::from_chars(x_str.begin(), x_str.end(), x);
        if (x <= 0 || x > 9) {
                std::cerr << "Invalid x components, should be between 1 and 9." << std::endl;
                return -2;
        }
        std::from_chars(y_str.begin(), y_str.end(), y);
        if (y <= 0 || y > 9) {
                std::cerr << "Invalid y components, should be between 1 and 9." << std::endl;
                return -2;
        }

        int width, height, n;
        unsigned char *data = stbi_load(argv[1], &width, &height, &n, 3);
        if (!data) {
                std::cerr << "Image loading failed for file '" << argv[1] << "'." << std::endl;
                return -1;
        }
        if (n != 3) {
                std::cerr << "Couldn't decode image to 3 channel rgb." << std::endl;
                return -1;
        }
        std::cout << blurhash::encode(data, width, height, x, y) << std::endl;

        stbi_image_free(data);

        return 0;
}
