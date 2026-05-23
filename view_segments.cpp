// view_segments.cpp
// Утилита для просмотра segments.bin

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <unordered_map>

int main() {
    std::ifstream inFile("segments.bin", std::ios::binary);
    if (!inFile) {
        std::cerr << "Ошибка открытия segments.bin" << std::endl;
        return 1;
    }

    // Заголовок
    int32_t num_segments, num_points;
    inFile.read(reinterpret_cast<char*>(&num_segments), sizeof(int32_t));
    inFile.read(reinterpret_cast<char*>(&num_points), sizeof(int32_t));

    std::cout << "=== Просмотр segments.bin ===" << std::endl;
    std::cout << "Сегментов: " << num_segments << std::endl;
    std::cout << "Уникальных точек: " << num_points << std::endl;
    std::cout << std::endl;

    // Точки
    std::unordered_map<int64_t, std::pair<double, double>> points;
    for (int i = 0; i < num_points; i++) {
        int64_t id;
        double x, y;
        inFile.read(reinterpret_cast<char*>(&id), sizeof(int64_t));
        inFile.read(reinterpret_cast<char*>(&x), sizeof(double));
        inFile.read(reinterpret_cast<char*>(&y), sizeof(double));
        points[id] = {x, y};
    }

    std::cout << "=== Точки пересечения ===" << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    for (const auto& [id, coords] : points) {
        std::cout << "  ID " << std::setw(10) << id << ": ("
                  << std::setw(8) << coords.first << ", "
                  << std::setw(8) << coords.second << ")";
        if (id >= 0) {
            std::cout << " [вершина " << id << "]";
        } else {
            int64_t encoded = -id - 1;
            int u = static_cast<int>(encoded / 1'000'000);
            int v = static_cast<int>(encoded % 1'000'000);
            std::cout << " [ребро " << u << "-" << v << "]";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Сегменты ===" << std::endl;
    for (int i = 0; i < num_segments; i++) {
        int64_t start_id, end_id;
        inFile.read(reinterpret_cast<char*>(&start_id), sizeof(int64_t));
        inFile.read(reinterpret_cast<char*>(&end_id), sizeof(int64_t));

        auto [x1, y1] = points[start_id];
        auto [x2, y2] = points[end_id];

        std::cout << "  S" << i << ": ID " << std::setw(10) << start_id
                  << " → ID " << std::setw(10) << end_id << std::endl;
        std::cout << "      (" << std::setw(8) << x1 << ", " << std::setw(8) << y1
                  << ") → (" << std::setw(8) << x2 << ", " << std::setw(8) << y2
                  << ")" << std::endl;
    }

    inFile.close();
    return 0;
}
