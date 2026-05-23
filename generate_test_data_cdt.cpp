// generate_test_data_cdt.cpp
// Оптимизированная версия с использованием CDT library
// Требует: CDT.h (https://github.com/artem-ogre/CDT)

#define CDT_USE_AS_COMPILED_LIBRARY
#include "CDT.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <set>
#include <map>

// Константы
const double PI = 3.14159265358979323846;
const double OUTER_MIN = -100.0;
const double OUTER_MAX = 100.0;
const double OUTER_STEP = 2.0;
const double STAR_OUTER_R = 25.0;
const double STAR_INNER_R = 10.0;
const double STAR_EDGE_STEP = 1.0;
const int NUM_STAR_POINTS = 5;
const int NUM_INTERIOR_POINTS = 50000;
const double Z_PLANE = 0.0;

// Структуры данных
struct Point {
    double x, y;
    Point(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
};

// Вычислительная геометрия
bool pointInPolygon(const Point& p, const std::vector<Point>& polygon) {
    int n = polygon.size();
    int count = 0;

    for (int i = 0; i < n; i++) {
        Point p1 = polygon[i];
        Point p2 = polygon[(i + 1) % n];

        if (p1.y == p2.y) continue;

        if (p.y < std::min(p1.y, p2.y)) continue;
        if (p.y >= std::max(p1.y, p2.y)) continue;

        double x = p1.x + (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);

        if (x > p.x) count++;
    }

    return (count % 2) == 1;
}

Point getCentroid(const Point& a, const Point& b, const Point& c) {
    return Point((a.x + b.x + c.x) / 3.0, (a.y + b.y + c.y) / 3.0);
}

double triangleSignedArea(const Point& a, const Point& b, const Point& c) {
    return 0.5 * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

// Генерация звезды
std::vector<Point> generateStarBoundary() {
    std::vector<Point> star;

    // Углы для 5 остриёв
    std::vector<double> outerAngles = {90.0, 162.0, 234.0, 306.0, 18.0};

    // Создаем 10 вершин звезды
    std::vector<Point> vertices;
    for (int i = 0; i < NUM_STAR_POINTS; i++) {
        double angleOuter = outerAngles[i] * PI / 180.0;
        vertices.push_back(Point(STAR_OUTER_R * cos(angleOuter),
                                STAR_OUTER_R * sin(angleOuter)));

        double angleInner = (outerAngles[i] + 36.0) * PI / 180.0;
        vertices.push_back(Point(STAR_INNER_R * cos(angleInner),
                                STAR_INNER_R * sin(angleInner)));
    }

    // Добавляем точки вдоль рёбер
    for (size_t i = 0; i < vertices.size(); i++) {
        Point p1 = vertices[i];
        Point p2 = vertices[(i + 1) % vertices.size()];

        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double len = sqrt(dx * dx + dy * dy);
        int numSteps = std::max(1, (int)(len / STAR_EDGE_STEP));

        for (int j = 0; j < numSteps; j++) {
            double t = (double)j / numSteps;
            star.push_back(Point(p1.x + t * dx, p1.y + t * dy));
        }
    }

    return star;
}

// Генерация внешней границы
std::vector<Point> generateOuterBoundary() {
    std::vector<Point> boundary;

    for (double x = OUTER_MIN; x <= OUTER_MAX; x += OUTER_STEP) {
        boundary.push_back(Point(x, OUTER_MIN));
    }

    for (double y = OUTER_MIN + OUTER_STEP; y <= OUTER_MAX; y += OUTER_STEP) {
        boundary.push_back(Point(OUTER_MAX, y));
    }

    for (double x = OUTER_MAX - OUTER_STEP; x >= OUTER_MIN; x -= OUTER_STEP) {
        boundary.push_back(Point(x, OUTER_MAX));
    }

    for (double y = OUTER_MAX - OUTER_STEP; y > OUTER_MIN; y -= OUTER_STEP) {
        boundary.push_back(Point(OUTER_MIN, y));
    }

    return boundary;
}

int main() {
    std::cout << "Генерация тестовых данных (с CDT)..." << std::endl;

    // 1. Генерация границ
    std::vector<Point> outerBoundary = generateOuterBoundary();
    std::vector<Point> starBoundary = generateStarBoundary();

    std::cout << "Внешняя граница: " << outerBoundary.size() << " точек" << std::endl;
    std::cout << "Граница звезды: " << starBoundary.size() << " точек" << std::endl;

    // 2. Генерация внутренних точек
    std::vector<Point> allPoints;
    allPoints.insert(allPoints.end(), outerBoundary.begin(), outerBoundary.end());

    int outerBoundarySize = outerBoundary.size();
    allPoints.insert(allPoints.end(), starBoundary.begin(), starBoundary.end());
    int starBoundarySize = starBoundary.size();

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(OUTER_MIN, OUTER_MAX);

    int accepted = 0;
    while (accepted < NUM_INTERIOR_POINTS) {
        Point p(dist(rng), dist(rng));

        if (!pointInPolygon(p, starBoundary)) {
            allPoints.push_back(p);
            accepted++;
        }

        if (accepted % 10000 == 0) {
            std::cout << "Сгенерировано точек: " << accepted << std::endl;
        }
    }

    std::cout << "Всего точек: " << allPoints.size() << std::endl;

    // 3. Подготовка данных для CDT
    std::vector<CDT::V2d<double>> vertices;
    for (const auto& p : allPoints) {
        vertices.push_back(CDT::V2d<double>(p.x, p.y));
    }

    // Внешняя граница (constraint)
    std::vector<CDT::Edge> outerEdges;
    for (size_t i = 0; i < outerBoundarySize; i++) {
        outerEdges.push_back(CDT::Edge(i, (i + 1) % outerBoundarySize));
    }

    // Граница звезды (constraint)
    std::vector<CDT::Edge> starEdges;
    for (size_t i = 0; i < starBoundarySize; i++) {
        size_t idx1 = outerBoundarySize + i;
        size_t idx2 = outerBoundarySize + ((i + 1) % starBoundarySize);
        starEdges.push_back(CDT::Edge(idx1, idx2));
    }

    // Объединяем constraints
    std::vector<CDT::Edge> edges = outerEdges;
    edges.insert(edges.end(), starEdges.begin(), starEdges.end());

    // 4. Триангуляция
    std::cout << "Запуск CDT триангуляции..." << std::endl;
    CDT::Triangulation<double> cdt(CDT::VertexInsertionOrder::Auto);

    cdt.insertVertices(vertices);
    cdt.insertEdges(edges);
    cdt.eraseOuterTrianglesAndHoles();

    const auto& triangles = cdt.triangles;
    std::cout << "Получено треугольников: " << triangles.size() << std::endl;

    // 5. Фильтрация треугольников
    struct Triangle {
        int v0, v1, v2;
    };

    std::vector<Triangle> validTriangles;
    for (const auto& t : triangles) {
        int v0 = t.vertices[0];
        int v1 = t.vertices[1];
        int v2 = t.vertices[2];

        Point centroid = getCentroid(allPoints[v0], allPoints[v1], allPoints[v2]);

        if (centroid.x >= OUTER_MIN && centroid.x <= OUTER_MAX &&
            centroid.y >= OUTER_MIN && centroid.y <= OUTER_MAX &&
            !pointInPolygon(centroid, starBoundary)) {
            validTriangles.push_back({v0, v1, v2});
        }
    }

    std::cout << "Треугольников после фильтрации: " << validTriangles.size() << std::endl;

    // Проверка ориентации (CCW)
    for (auto& t : validTriangles) {
        double area = triangleSignedArea(allPoints[t.v0], allPoints[t.v1], allPoints[t.v2]);
        if (area < 0) {
            std::swap(t.v1, t.v2);
        }
    }

    // 6. Вычисление высот и масок
    int numVertices = allPoints.size();
    std::vector<double> zValues(numVertices);
    std::vector<uint8_t> boundaryMask(numVertices, 0);
    std::vector<uint8_t> plateauMask(numVertices, 0);
    std::vector<uint8_t> ridgeMask(numVertices, 0);

    // Базовый рельеф
    for (int i = 0; i < numVertices; i++) {
        double x = allPoints[i].x;
        double y = allPoints[i].y;
        zValues[i] = 10.0 * sin(x / 15.0) * cos(y / 15.0);
    }

    // Граничные вершины
    for (int i = 0; i < outerBoundarySize + starBoundarySize; i++) {
        zValues[i] = 1000.0;
        boundaryMask[i] = 1;
    }

    // Плато
    for (int i = 0; i < numVertices; i++) {
        double x = allPoints[i].x;
        double y = allPoints[i].y;
        double rSq = x * x + y * y;

        if (rSq >= 35.0 * 35.0 && rSq <= 45.0 * 45.0) {
            zValues[i] = 0.0;
            plateauMask[i] = 1;
        }
    }

    // Хребет
    std::vector<int> ridgeVertices;
    for (int i = 0; i < numVertices; i++) {
        if (boundaryMask[i]) continue;

        double x = allPoints[i].x;
        double y = allPoints[i].y;

        if (x >= -80.0 && x <= -60.0 && fabs(y - 80.0) <= 2.0) {
            zValues[i] = 0.0;
            ridgeMask[i] = 1;
            ridgeVertices.push_back(i);
        }
    }

    // Граф смежности
    std::map<int, std::set<int>> adjacency;
    for (const auto& t : validTriangles) {
        adjacency[t.v0].insert(t.v1);
        adjacency[t.v0].insert(t.v2);
        adjacency[t.v1].insert(t.v0);
        adjacency[t.v1].insert(t.v2);
        adjacency[t.v2].insert(t.v0);
        adjacency[t.v2].insert(t.v1);
    }

    // Соседи хребта
    for (int ridgeIdx : ridgeVertices) {
        for (int neighbor : adjacency[ridgeIdx]) {
            if (!boundaryMask[neighbor] && !ridgeMask[neighbor] && zValues[neighbor] < 0.0) {
                zValues[neighbor] = -5.0;
            }
        }
    }

    // Изолированный пик
    int isolatedPeakIndex = -1;
    for (int i = 0; i < numVertices; i++) {
        if (boundaryMask[i]) continue;

        double x = allPoints[i].x;
        double y = allPoints[i].y;
        double rSq = x * x + y * y;
        double zBase = 10.0 * sin(x / 15.0) * cos(y / 15.0);

        if (zBase < -3.0 && rSq > 50.0 * 50.0 &&
            x >= -80.0 && x <= -60.0 && y >= -80.0 && y <= -60.0) {
            isolatedPeakIndex = i;
            zValues[i] = 0.0;
            break;
        }
    }

    std::cout << "Изолированный пик: " << (isolatedPeakIndex >= 0 ? "найден" : "не найден") << std::endl;

    // 7. Сохранение
    std::ofstream outFile("test_data.bin", std::ios::binary);
    if (!outFile) {
        std::cerr << "Ошибка создания файла" << std::endl;
        return 1;
    }

    int32_t N = numVertices;
    int32_t M = validTriangles.size();

    outFile.write(reinterpret_cast<const char*>(&N), sizeof(int32_t));
    outFile.write(reinterpret_cast<const char*>(&M), sizeof(int32_t));

    for (const auto& p : allPoints) {
        outFile.write(reinterpret_cast<const char*>(&p.x), sizeof(double));
        outFile.write(reinterpret_cast<const char*>(&p.y), sizeof(double));
    }

    outFile.write(reinterpret_cast<const char*>(zValues.data()), N * sizeof(double));

    for (const auto& t : validTriangles) {
        outFile.write(reinterpret_cast<const char*>(&t.v0), sizeof(int32_t));
        outFile.write(reinterpret_cast<const char*>(&t.v1), sizeof(int32_t));
        outFile.write(reinterpret_cast<const char*>(&t.v2), sizeof(int32_t));
    }

    outFile.write(reinterpret_cast<const char*>(&Z_PLANE), sizeof(double));
    outFile.write(reinterpret_cast<const char*>(boundaryMask.data()), N);
    outFile.write(reinterpret_cast<const char*>(plateauMask.data()), N);
    outFile.write(reinterpret_cast<const char*>(ridgeMask.data()), N);
    outFile.write(reinterpret_cast<const char*>(&isolatedPeakIndex), sizeof(int32_t));

    outFile.close();
    std::cout << "Данные сохранены в test_data.bin" << std::endl;

    // 8. Отчёт
    std::ofstream reportFile("test_data_report.txt");
    if (reportFile) {
        reportFile << "=== Отчёт о тестовых данных ===" << std::endl;
        reportFile << "Количество вершин: " << N << std::endl;
        reportFile << "Количество треугольников: " << M << std::endl;
        reportFile << "Граничных вершин: " << std::count(boundaryMask.begin(), boundaryMask.end(), 1) << std::endl;
        reportFile << "Вершин плато: " << std::count(plateauMask.begin(), plateauMask.end(), 1) << std::endl;
        reportFile << "Вершин хребта: " << std::count(ridgeMask.begin(), ridgeMask.end(), 1) << std::endl;

        if (isolatedPeakIndex >= 0) {
            reportFile << "Изолированный пик: индекс " << isolatedPeakIndex
                      << " at (" << allPoints[isolatedPeakIndex].x
                      << ", " << allPoints[isolatedPeakIndex].y << ")" << std::endl;
        } else {
            reportFile << "Изолированный пик: не найден" << std::endl;
        }

        reportFile << "Уровень сечения z_plane: " << Z_PLANE << std::endl;
        reportFile.close();
        std::cout << "Отчёт сохранён в test_data_report.txt" << std::endl;
    }

    std::cout << "Готово!" << std::endl;
    return 0;
}
