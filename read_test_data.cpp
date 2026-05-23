// read_test_data.cpp
// Утилита для чтения и проверки test_data.bin

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstdint>

struct Point {
    double x, y;
};

struct Triangle {
    int32_t v0, v1, v2;
};

int main() {
    std::ifstream inFile("test_data.bin", std::ios::binary);
    if (!inFile) {
        std::cerr << "Ошибка открытия test_data.bin" << std::endl;
        std::cerr << "Сначала запустите генератор: generate_test_data.exe" << std::endl;
        return 1;
    }

    // Чтение заголовка
    int32_t N, M;
    inFile.read(reinterpret_cast<char*>(&N), sizeof(int32_t));
    inFile.read(reinterpret_cast<char*>(&M), sizeof(int32_t));

    std::cout << "=== Загрузка test_data.bin ===" << std::endl;
    std::cout << "Вершин: " << N << std::endl;
    std::cout << "Треугольников: " << M << std::endl;
    std::cout << std::endl;

    // Чтение вершин
    std::vector<Point> vertices(N);
    for (int32_t i = 0; i < N; i++) {
        inFile.read(reinterpret_cast<char*>(&vertices[i].x), sizeof(double));
        inFile.read(reinterpret_cast<char*>(&vertices[i].y), sizeof(double));
    }

    // Чтение высот
    std::vector<double> zValues(N);
    inFile.read(reinterpret_cast<char*>(zValues.data()), N * sizeof(double));

    // Чтение треугольников
    std::vector<Triangle> triangles(M);
    inFile.read(reinterpret_cast<char*>(triangles.data()), M * sizeof(Triangle));

    // z_plane
    double z_plane;
    inFile.read(reinterpret_cast<char*>(&z_plane), sizeof(double));

    // Маски
    std::vector<uint8_t> boundaryMask(N);
    std::vector<uint8_t> plateauMask(N);
    std::vector<uint8_t> ridgeMask(N);

    inFile.read(reinterpret_cast<char*>(boundaryMask.data()), N);
    inFile.read(reinterpret_cast<char*>(plateauMask.data()), N);
    inFile.read(reinterpret_cast<char*>(ridgeMask.data()), N);

    // Индекс изолированного пика
    int32_t isolatedPeakIndex;
    inFile.read(reinterpret_cast<char*>(&isolatedPeakIndex), sizeof(int32_t));

    inFile.close();

    // Статистика
    int boundaryCount = 0;
    int plateauCount = 0;
    int ridgeCount = 0;

    double minZ = zValues[0], maxZ = zValues[0];
    double minX = vertices[0].x, maxX = vertices[0].x;
    double minY = vertices[0].y, maxY = vertices[0].y;

    for (int32_t i = 0; i < N; i++) {
        if (boundaryMask[i]) boundaryCount++;
        if (plateauMask[i]) plateauCount++;
        if (ridgeMask[i]) ridgeCount++;

        if (zValues[i] < minZ) minZ = zValues[i];
        if (zValues[i] > maxZ) maxZ = zValues[i];
        if (vertices[i].x < minX) minX = vertices[i].x;
        if (vertices[i].x > maxX) maxX = vertices[i].x;
        if (vertices[i].y < minY) minY = vertices[i].y;
        if (vertices[i].y > maxY) maxY = vertices[i].y;
    }

    std::cout << "=== Статистика ===" << std::endl;
    std::cout << "Граничных вершин: " << boundaryCount << std::endl;
    std::cout << "Вершин плато: " << plateauCount << std::endl;
    std::cout << "Вершин хребта: " << ridgeCount << std::endl;
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Диапазон X: [" << minX << ", " << maxX << "]" << std::endl;
    std::cout << "Диапазон Y: [" << minY << ", " << maxY << "]" << std::endl;
    std::cout << "Диапазон Z: [" << minZ << ", " << maxZ << "]" << std::endl;
    std::cout << "Уровень сечения z_plane: " << z_plane << std::endl;
    std::cout << std::endl;

    if (isolatedPeakIndex >= 0) {
        std::cout << "Изолированный пик:" << std::endl;
        std::cout << "  Индекс: " << isolatedPeakIndex << std::endl;
        std::cout << "  Координаты: (" << vertices[isolatedPeakIndex].x
                  << ", " << vertices[isolatedPeakIndex].y << ")" << std::endl;
        std::cout << "  Высота: " << zValues[isolatedPeakIndex] << std::endl;
    } else {
        std::cout << "Изолированный пик: не найден" << std::endl;
    }
    std::cout << std::endl;

    // Проверка корректности треугольников
    int invalidTriangles = 0;
    for (int32_t i = 0; i < M; i++) {
        if (triangles[i].v0 < 0 || triangles[i].v0 >= N ||
            triangles[i].v1 < 0 || triangles[i].v1 >= N ||
            triangles[i].v2 < 0 || triangles[i].v2 >= N) {
            invalidTriangles++;
        }
    }

    std::cout << "=== Проверка корректности ===" << std::endl;
    if (invalidTriangles == 0) {
        std::cout << "✓ Все треугольники корректны" << std::endl;
    } else {
        std::cout << "✗ Некорректных треугольников: " << invalidTriangles << std::endl;
    }

    // Примеры данных
    std::cout << std::endl;
    std::cout << "=== Примеры вершин ===" << std::endl;
    for (int i = 0; i < std::min(5, N); i++) {
        std::cout << "V" << i << ": (" << std::setw(8) << vertices[i].x
                  << ", " << std::setw(8) << vertices[i].y
                  << ") z=" << std::setw(8) << zValues[i];

        if (boundaryMask[i]) std::cout << " [ГРАНИЦА]";
        if (plateauMask[i]) std::cout << " [ПЛАТО]";
        if (ridgeMask[i]) std::cout << " [ХРЕБЕТ]";

        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Примеры треугольников ===" << std::endl;
    for (int i = 0; i < std::min(5, M); i++) {
        std::cout << "T" << i << ": ("
                  << triangles[i].v0 << ", "
                  << triangles[i].v1 << ", "
                  << triangles[i].v2 << ")" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "✓ Данные успешно загружены и проверены!" << std::endl;

    return 0;
}
