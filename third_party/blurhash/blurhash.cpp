#include "blurhash.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#if __has_include(<doctest.h>)
#include <doctest.h>
#else
#include <doctest/doctest.h>
#endif
#endif

using namespace std::literals;

namespace {
template<class T>
T pi = 3.14159265358979323846;

constexpr std::array<char, 84> int_to_b83{
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~"};

std::string
leftPad(std::string str, size_t len)
{
        if (str.size() >= len)
                return str;
        return str.insert(0, len - str.size(), '0');
}

constexpr std::array<int, 255> b83_to_int = []() constexpr
{
        std::array<int, 255> a{};

        for (auto &e : a)
                e = -1;

        for (int i = 0; i < 83; i++) {
                a[static_cast<unsigned char>(int_to_b83[i])] = i;
        }

        return a;
}
();

std::string
encode83(int value)
{
        std::string buffer;

        do {
                buffer += int_to_b83[value % 83];
        } while ((value = value / 83));

        std::reverse(buffer.begin(), buffer.end());
        return buffer;
}

struct Components
{
        int x, y;
};

int
packComponents(const Components &c) noexcept
{
        return (c.x - 1) + (c.y - 1) * 9;
}

Components
unpackComponents(int c) noexcept
{
        return {c % 9 + 1, c / 9 + 1};
}

int
decode83(std::string_view value)
{
        int temp = 0;

        for (char c : value)
                if (b83_to_int[static_cast<unsigned char>(c)] < 0)
                        throw std::invalid_argument("invalid character in blurhash");

        for (char c : value)
                temp = temp * 83 + b83_to_int[static_cast<unsigned char>(c)];
        return temp;
}

float
decodeMaxAC(int quantizedMaxAC) noexcept
{
        return (quantizedMaxAC + 1) / 166.;
}

float
decodeMaxAC(std::string_view maxAC)
{
        assert(maxAC.size() == 1);
        return decodeMaxAC(decode83(maxAC));
}

int
encodeMaxAC(float maxAC) noexcept
{
        return std::max(0, std::min(82, int(maxAC * 166 - 0.5f)));
}

float
srgbToLinear(int value) noexcept
{
        auto srgbToLinearF = [](float x) {
                if (x <= 0.0f)
                        return 0.0f;
                else if (x >= 1.0f)
                        return 1.0f;
                else if (x < 0.04045f)
                        return x / 12.92f;
                else
                        return std::pow((x + 0.055f) / 1.055f, 2.4f);
        };

        return srgbToLinearF(value / 255.f);
}

int
linearToSrgb(float value) noexcept
{
        auto linearToSrgbF = [](float x) -> float {
                if (x <= 0.0f)
                        return 0.0f;
                else if (x >= 1.0f)
                        return 1.0f;
                else if (x < 0.0031308f)
                        return x * 12.92f;
                else
                        return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
        };

        return int(linearToSrgbF(value) * 255.f + 0.5f);
}

struct Color
{
        float r, g, b;

        Color &operator*=(float scale)
        {
                r *= scale;
                g *= scale;
                b *= scale;
                return *this;
        }
        friend Color operator*(Color lhs, float rhs) { return (lhs *= rhs); }
        Color &operator/=(float scale)
        {
                r /= scale;
                g /= scale;
                b /= scale;
                return *this;
        }
        Color &operator+=(const Color &rhs)
        {
                r += rhs.r;
                g += rhs.g;
                b += rhs.b;
                return *this;
        }
};

Color
decodeDC(int value)
{
        const int intR = value >> 16;
        const int intG = (value >> 8) & 255;
        const int intB = value & 255;
        return {srgbToLinear(intR), srgbToLinear(intG), srgbToLinear(intB)};
}

Color
decodeDC(std::string_view value)
{
        assert(value.size() == 4);
        return decodeDC(decode83(value));
}

int
encodeDC(const Color &c)
{
        return (linearToSrgb(c.r) << 16) + (linearToSrgb(c.g) << 8) + linearToSrgb(c.b);
}

float
signPow(float value, float exp)
{
        return std::copysign(std::pow(std::abs(value), exp), value);
}

int
encodeAC(const Color &c, float maximumValue)
{
        auto quantR =
          int(std::max(0., std::min(18., std::floor(signPow(c.r / maximumValue, 0.5) * 9 + 9.5))));
        auto quantG =
          int(std::max(0., std::min(18., std::floor(signPow(c.g / maximumValue, 0.5) * 9 + 9.5))));
        auto quantB =
          int(std::max(0., std::min(18., std::floor(signPow(c.b / maximumValue, 0.5) * 9 + 9.5))));

        return quantR * 19 * 19 + quantG * 19 + quantB;
}

Color
decodeAC(int value, float maximumValue)
{
        auto quantR = value / (19 * 19);
        auto quantG = (value / 19) % 19;
        auto quantB = value % 19;

        return {signPow((float(quantR) - 9) / 9, 2) * maximumValue,
                signPow((float(quantG) - 9) / 9, 2) * maximumValue,
                signPow((float(quantB) - 9) / 9, 2) * maximumValue};
}

Color
decodeAC(std::string_view value, float maximumValue)
{
        return decodeAC(decode83(value), maximumValue);
}

std::vector<float>
bases_for(size_t dimension, size_t components)
{
        std::vector<float> bases(dimension * components, 0.f);
        auto scale = pi<float> / float(dimension);
        for (size_t x = 0; x < dimension; x++) {
                for (size_t nx = 0; nx < size_t(components); nx++) {
                        bases[x * components + nx] = std::cos(scale * float(nx * x));
                }
        }
        return bases;
}
}

namespace blurhash {
Image
decode(std::string_view blurhash, size_t width, size_t height, size_t bytesPerPixel) noexcept
{
        Image i{};

        if (blurhash.size() < 10)
                return i;

        Components components{};
        std::vector<Color> values;
        values.reserve(blurhash.size() / 2);
        try {
                components = unpackComponents(decode83(blurhash.substr(0, 1)));

                if (components.x < 1 || components.y < 1 ||
                    blurhash.size() != size_t(1 + 1 + 4 + (components.x * components.y - 1) * 2))
                        return {};

                auto maxAC    = decodeMaxAC(blurhash.substr(1, 1));
                Color average = decodeDC(blurhash.substr(2, 4));

                values.push_back(average);
                for (size_t c = 6; c < blurhash.size(); c += 2)
                        values.push_back(decodeAC(blurhash.substr(c, 2), maxAC));
        } catch (std::invalid_argument &) {
                return {};
        }

        i.image = decltype(i.image)(height * width * bytesPerPixel, 255);

        std::vector<float> basis_x = bases_for(width, components.x);
        std::vector<float> basis_y = bases_for(height, components.y);

        for (size_t y = 0; y < height; y++) {
                for (size_t x = 0; x < width; x++) {
                        Color c{};

                        for (size_t nx = 0; nx < size_t(components.x); nx++) {
                                for (size_t ny = 0; ny < size_t(components.y); ny++) {
                                        float basis = basis_x[x * components.x + nx] *
                                                      basis_y[y * components.y + ny];
                                        c += values[nx + ny * components.x] * basis;
                                }
                        }

                        i.image[(y * width + x) * bytesPerPixel + 0] =
                          static_cast<unsigned char>(linearToSrgb(c.r));
                        i.image[(y * width + x) * bytesPerPixel + 1] =
                          static_cast<unsigned char>(linearToSrgb(c.g));
                        i.image[(y * width + x) * bytesPerPixel + 2] =
                          static_cast<unsigned char>(linearToSrgb(c.b));
                }
        }

        i.height = height;
        i.width  = width;

        return i;
}

std::string
encode(unsigned char *image, size_t width, size_t height, int components_x, int components_y)
{
        if (width < 1 || height < 1 || components_x < 1 || components_x > 9 || components_y < 1 ||
            components_y > 9 || !image)
                return "";

        std::vector<float> basis_x = bases_for(width, components_x);
        std::vector<float> basis_y = bases_for(height, components_y);

        std::vector<Color> factors(components_x * components_y, Color{});
        for (size_t y = 0; y < height; y++) {
                for (size_t x = 0; x < width; x++) {
                        Color linear{srgbToLinear(image[3 * x + 0 + y * width * 3]),
                                     srgbToLinear(image[3 * x + 1 + y * width * 3]),
                                     srgbToLinear(image[3 * x + 2 + y * width * 3])};

                        // other half of normalization.
                        linear *= 1.f / width;

                        for (size_t ny = 0; ny < size_t(components_y); ny++) {
                                for (size_t nx = 0; nx < size_t(components_x); nx++) {
                                        float basis = basis_x[x * size_t(components_x) + nx] *
                                                      basis_y[y * size_t(components_y) + ny];
                                        factors[ny * components_x + nx] += linear * basis;
                                }
                        }
                }
        }

        // scale by normalization. Half the scaling is done in the previous loop to prevent going
        // too far outside the float range.
        for (size_t i = 0; i < factors.size(); i++) {
                float normalisation = (i == 0) ? 1 : 2;
                float scale         = normalisation / (height);
                factors[i] *= scale;
        }

        assert(factors.size() > 0);

        auto dc = factors.front();
        factors.erase(factors.begin());

        std::string h;

        h += leftPad(encode83(packComponents({components_x, components_y})), 1);

        float maximumValue;
        if (!factors.empty()) {
                float actualMaximumValue = 0;
                for (auto ac : factors) {
                        actualMaximumValue = std::max({
                          std::abs(ac.r),
                          std::abs(ac.g),
                          std::abs(ac.b),
                          actualMaximumValue,
                        });
                }

                int quantisedMaximumValue = encodeMaxAC(actualMaximumValue);
                maximumValue              = ((float)quantisedMaximumValue + 1) / 166;
                h += leftPad(encode83(quantisedMaximumValue), 1);
        } else {
                maximumValue = 1;
                h += leftPad(encode83(0), 1);
        }

        h += leftPad(encode83(encodeDC(dc)), 4);

        for (auto ac : factors)
                h += leftPad(encode83(encodeAC(ac, maximumValue)), 2);

        return h;
}
}

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
TEST_CASE("component packing")
{
        for (int i = 0; i < 9 * 9; i++)
                CHECK(packComponents(unpackComponents(i)) == i);
}

TEST_CASE("encode83")
{
        CHECK(encode83(0) == "0");

        CHECK(encode83(packComponents({4, 3})) == "L");
        CHECK(encode83(packComponents({4, 4})) == "U");
        CHECK(encode83(packComponents({8, 4})) == "Y");
        CHECK(encode83(packComponents({2, 1})) == "1");
}

TEST_CASE("decode83")
{
        CHECK(packComponents({4, 3}) == decode83("L"));
        CHECK(packComponents({4, 4}) == decode83("U"));
        CHECK(packComponents({8, 4}) == decode83("Y"));
        CHECK(packComponents({2, 1}) == decode83("1"));
}

TEST_CASE("maxAC")
{
        for (int i = 0; i < 83; i++)
                CHECK(encodeMaxAC(decodeMaxAC(i)) == i);

        CHECK(std::abs(decodeMaxAC("l"sv) - 0.289157f) < 0.00001f);
}

TEST_CASE("DC")
{
        CHECK(encode83(encodeDC(decodeDC("MF%n"))) == "MF%n"sv);
        CHECK(encode83(encodeDC(decodeDC("HV6n"))) == "HV6n"sv);
        CHECK(encode83(encodeDC(decodeDC("F5]+"))) == "F5]+"sv);
        CHECK(encode83(encodeDC(decodeDC("Pj0^"))) == "Pj0^"sv);
        CHECK(encode83(encodeDC(decodeDC("O2?U"))) == "O2?U"sv);
}

TEST_CASE("AC")
{
        auto h = "00%#MwS|WCWEM{R*bbWBbH"sv;
        for (size_t i = 0; i < h.size(); i += 2) {
                auto s           = h.substr(i, 2);
                const auto maxAC = 0.289157f;
                CHECK(leftPad(encode83(encodeAC(decodeAC(decode83(s), maxAC), maxAC)), 2) == s);
        }
}

TEST_CASE("decode")
{
        blurhash::Image i1 = blurhash::decode("LEHV6nWB2yk8pyoJadR*.7kCMdnj", 360, 200);
        CHECK(i1.width == 360);
        CHECK(i1.height == 200);
        CHECK(i1.image.size() == i1.height * i1.width * 3);
        CHECK(i1.image[0] == 135);
        CHECK(i1.image[1] == 164);
        CHECK(i1.image[2] == 177);
        CHECK(i1.image[10000] == 173);
        CHECK(i1.image[10001] == 176);
        CHECK(i1.image[10002] == 163);
        // stbi_write_bmp("test.bmp", i1.width, i1.height, 3, (void *)i1.image.data());

        i1 = blurhash::decode("LGF5]+Yk^6#M@-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 360);
        CHECK(i1.height == 200);
        CHECK(i1.image.size() == i1.height * i1.width * 3);
        // stbi_write_bmp("test2.bmp", i1.width, i1.height, 3, (void *)i1.image.data());

        // invalid inputs
        i1 = blurhash::decode(" LGF5]+Yk^6#M@-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 0);
        CHECK(i1.height == 0);
        CHECK(i1.image.size() == 0);
        i1 = blurhash::decode("  LGF5]+Yk^6#M@-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 0);
        CHECK(i1.height == 0);
        CHECK(i1.image.size() == 0);

        i1 = blurhash::decode("LGF5]+Yk^6# M@-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 0);
        CHECK(i1.height == 0);
        CHECK(i1.image.size() == 0);
        i1 = blurhash::decode("LGF5]+Yk^6#  M@-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 0);
        CHECK(i1.height == 0);
        CHECK(i1.image.size() == 0);

        i1 = blurhash::decode("LGF5]+Yk^6# @-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 0);
        CHECK(i1.height == 0);
        CHECK(i1.image.size() == 0);
        i1 = blurhash::decode(" GF5]+Yk^6#M@-5c,1J5@[or[Q6.", 360, 200);
        CHECK(i1.width == 0);
        CHECK(i1.height == 0);
        CHECK(i1.image.size() == 0);
}

TEST_CASE("encode")
{
        CHECK(blurhash::encode(nullptr, 360, 200, 4, 3) == "");

        std::vector<unsigned char> black(360 * 200 * 3, 0);
        CHECK(blurhash::encode(black.data(), 0, 200, 4, 3) == "");
        CHECK(blurhash::encode(black.data(), 360, 0, 4, 3) == "");
        CHECK(blurhash::encode(black.data(), 360, 200, 0, 3) == "");
        CHECK(blurhash::encode(black.data(), 360, 200, 4, 0) == "");
        CHECK(blurhash::encode(black.data(), 360, 200, 4, 3) == "L00000fQfQfQfQfQfQfQfQfQfQfQ");
}
#endif
