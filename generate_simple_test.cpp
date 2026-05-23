// generate_simple_test.cpp
// Создает минимальный test_data.bin для быстрого тестирования

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cmath>

int main() {
    // Простая геометрия: квадрат разбитый на треугольники
    // 9 вершин в сетке 3x3, высоты варьируются

    std::vector<double> x_coords = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
    std::vector<double> y_coords = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
    std::vector<double> z_coords = {-2, -1, -2, -1, 0, -1, -2, 1, -2};

    // 8 треугольников (2 на каждый квадрат)
    std::vector<int32_t> triangles = {
        0,1,3,  1,4,3,  // нижний ряд
        1,2,4,  2,5,4,
        3,4,6,  4,7,6,  // верхний ряд
        4,5,7,  5,8,7
    };

    int32_t N = 9;
    int32_t M = 8;
    double z_plane = 0.0;

    std::ofstream out("test_data.bin", std::ios::binary);

    // Заголовок
    out.write(reinterpret_cast<const char*>(&N), sizeof(int32_t));
    out.write(reinterpret_cast<const char*>(&M), sizeof(int32_t));

    // Координаты вершин
    for (int i = 0; i < N; i++) {
        out.write(reinterpret_cast<const char*>(&x_coords[i]), sizeof(double));
        out.write(reinterpret_cast<const char*>(&y_coords[i]), sizeof(double));
    }

    // Z-координаты
    out.write(reinterpret_cast<const char*>(z_coords.data()), N * sizeof(double));

    // Треугольники
    out.write(reinterpret_cast<const char*>(triangles.data()), M * 3 * sizeof(int32_t));

    // z_plane
    out.write(reinterpret_cast<const char*>(&z_plane), sizeof(double));

    // Маски (заглушки)
    std::vector<uint8_t> mask(N, 0);
    out.write(reinterpret_cast<const char*>(mask.data()), N);
    out.write(reinterpret_cast<const char*>(mask.data()), N);
    out.write(reinterpret_cast<const char*>(mask.data()), N);

    // Изолированный пик
    int32_t peak = -1;
    out.write(reinterpret_cast<const char*>(&peak), sizeof(int32_t));

    out.close();

    std::cout << "Создан test_data.bin (9 вершин, 8 треугольников)" << std::endl;
    std::cout << "Z-координаты варьируются от -2 до 1" << std::endl;
    std::cout << "Уровень сечения z_plane = 0.0" << std::endl;

    return 0;
}
