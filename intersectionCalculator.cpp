// intersectionCalculator.cpp
// Основное приложение для вычисления изолиний на триангулированной поверхности
// Реализация варианта B: шаги 1-4 алгоритма изолиний

#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Структуры данных
struct Point {
    double x, y;

    Point(double x_ = 0.0, double y_ = 0.0) : x(x_), y(y_) {}
};

struct Triangle {
    int32_t v0, v1, v2;
};

struct Segment {
    int64_t start_id, end_id;

    Segment(int64_t s = 0, int64_t e = 0) : start_id(s), end_id(e) {}

    bool operator==(const Segment& other) const {
        return start_id == other.start_id && end_id == other.end_id;
    }
};

// Кодирование ID точек пересечения
int64_t encode_vertex(int u) {
    return static_cast<int64_t>(u);
}

int64_t encode_edge(int u, int v) {
    if (u > v) std::swap(u, v);
    return -(static_cast<int64_t>(u) * 1'000'000LL + static_cast<int64_t>(v) + 1);
}

// Глобальный кэш точек пересечения
std::unordered_map<int64_t, Point> point_cache;

// Получение или создание точки по ID
Point get_or_create_point(
    int64_t id,
    const std::vector<Point>& vertices,
    const std::vector<double>& zValues,
    double z_plane
) {
    auto it = point_cache.find(id);
    if (it != point_cache.end()) {
        return it->second;
    }

    Point p;

    if (id >= 0) {
        // Точка совпадает с вершиной
        p = vertices[id];
    } else {
        // Точка на ребре
        int64_t encoded = -id - 1;
        int u = static_cast<int>(encoded / 1'000'000LL);
        int v = static_cast<int>(encoded % 1'000'000LL);

        // Линейная интерполяция
        double z_u = zValues[u];
        double z_v = zValues[v];
        double t = (z_plane - z_u) / (z_v - z_u);

        p.x = vertices[u].x + t * (vertices[v].x - vertices[u].x);
        p.y = vertices[u].y + t * (vertices[v].y - vertices[u].y);
    }

    point_cache[id] = p;
    return p;
}

// Шаг 1: Бинаризация вершин
std::vector<int8_t> binarize(const std::vector<double>& zValues, double z_plane) {
    std::vector<int8_t> state(zValues.size());

    for (size_t i = 0; i < zValues.size(); ++i) {
        state[i] = (zValues[i] >= z_plane) ? 1 : 0;
    }

    return state;
}

// Шаг 3: Генерация ориентированных сегментов
std::vector<Segment> generate_segments(
    const std::vector<Triangle>& triangles,
    const std::vector<int8_t>& state,
    const std::vector<Point>& vertices,
    const std::vector<double>& zValues,
    double z_plane
) {
    std::vector<Segment> segments;

    for (const auto& tri : triangles) {
        int32_t v[3] = {tri.v0, tri.v1, tri.v2};
        int8_t s[3] = {state[v[0]], state[v[1]], state[v[2]]};

        // Сумма состояний
        int sum = s[0] + s[1] + s[2];

        // Пропускаем треугольники без пересечений
        if (sum == 0 || sum == 3) {
            continue;
        }

        // Находим "одиночную" вершину (отличается от двух других)
        int single_idx = -1;

        if (sum == 1) {
            // Одна вершина выше (s=1), две ниже (s=0)
            for (int i = 0; i < 3; ++i) {
                if (s[i] == 1) {
                    single_idx = i;
                    break;
                }
            }
        } else {
            // sum == 2: две вершины выше (s=1), одна ниже (s=0)
            for (int i = 0; i < 3; ++i) {
                if (s[i] == 0) {
                    single_idx = i;
                    break;
                }
            }
        }

        if (single_idx == -1) continue;

        int v_i = v[single_idx];
        int v_next = v[(single_idx + 1) % 3];
        int v_prev = v[(single_idx + 2) % 3];

        // Вычисляем ID точек пересечения
        int64_t id_a, id_b;

        // Ребро A: v_i → v_next
        double z_i = zValues[v_i];
        double z_next = zValues[v_next];

        if (std::abs(z_i - z_plane) < 1e-10) {
            id_a = encode_vertex(v_i);
        } else if (std::abs(z_next - z_plane) < 1e-10) {
            id_a = encode_vertex(v_next);
        } else {
            id_a = encode_edge(v_i, v_next);
        }

        // Ребро B: v_prev → v_i
        double z_prev = zValues[v_prev];

        if (std::abs(z_prev - z_plane) < 1e-10) {
            id_b = encode_vertex(v_prev);
        } else if (std::abs(z_i - z_plane) < 1e-10) {
            id_b = encode_vertex(v_i);
        } else {
            id_b = encode_edge(v_prev, v_i);
        }

        // Вычисляем точки для кэша
        get_or_create_point(id_a, vertices, zValues, z_plane);
        get_or_create_point(id_b, vertices, zValues, z_plane);

        // Направление сегмента зависит от состояния одиночной вершины
        if (s[single_idx] == 0) {
            // Одиночная вершина ниже → сегмент P_a → P_b
            segments.emplace_back(id_a, id_b);
        } else {
            // Одиночная вершина выше → сегмент P_b → P_a
            segments.emplace_back(id_b, id_a);
        }
    }

    return segments;
}

// Шаг 4: Фильтрация вырождений
std::vector<Segment> filter_segments(
    const std::vector<Segment>& segments,
    int& loops_removed,
    int& pairs_annihilated
) {
    loops_removed = 0;
    pairs_annihilated = 0;

    // Удаление петель
    std::vector<Segment> no_loops;
    for (const auto& seg : segments) {
        if (seg.start_id == seg.end_id) {
            loops_removed++;
        } else {
            no_loops.push_back(seg);
        }
    }

    // Аннигиляция: подсчёт баланса для каждой направленной пары
    std::unordered_map<int64_t, std::unordered_map<int64_t, int>> balance;

    for (const auto& seg : no_loops) {
        balance[seg.start_id][seg.end_id]++;
    }

    // Вычитаем обратные пары
    std::vector<Segment> filtered;
    std::unordered_map<int64_t, std::unordered_map<int64_t, bool>> processed;

    for (const auto& [start, ends] : balance) {
        for (const auto& [end, count] : ends) {
            if (processed[start][end]) continue;

            int reverse_count = balance[end][start];
            int net_count = count - reverse_count;

            if (net_count > 0) {
                // Добавляем net_count копий сегмента start → end
                for (int i = 0; i < net_count; ++i) {
                    filtered.emplace_back(start, end);
                }
            }

            // Помечаем обе пары как обработанные
            processed[start][end] = true;
            processed[end][start] = true;

            // Считаем аннигилированные пары
            int min_count = std::min(count, reverse_count);
            pairs_annihilated += min_count;
        }
    }

    return filtered;
}

// Сохранение результата в segments.bin
void save_segments(
    const std::string& filename,
    const std::vector<Segment>& segments
) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Ошибка создания файла " << filename << std::endl;
        return;
    }

    // Заголовок
    int32_t num_segments = static_cast<int32_t>(segments.size());
    int32_t num_points = static_cast<int32_t>(point_cache.size());

    outFile.write(reinterpret_cast<const char*>(&num_segments), sizeof(int32_t));
    outFile.write(reinterpret_cast<const char*>(&num_points), sizeof(int32_t));

    // Точки (ID → координаты)
    for (const auto& [id, point] : point_cache) {
        outFile.write(reinterpret_cast<const char*>(&id), sizeof(int64_t));
        outFile.write(reinterpret_cast<const char*>(&point.x), sizeof(double));
        outFile.write(reinterpret_cast<const char*>(&point.y), sizeof(double));
    }

    // Сегменты (start_id, end_id)
    for (const auto& seg : segments) {
        outFile.write(reinterpret_cast<const char*>(&seg.start_id), sizeof(int64_t));
        outFile.write(reinterpret_cast<const char*>(&seg.end_id), sizeof(int64_t));
    }

    outFile.close();
    std::cout << "Результаты сохранены в " << filename << std::endl;
}

// ============================================================================
// Вариант C: Построение графа, разрешение седловых точек, сборка полилайнов
// ============================================================================

// Структура графа
struct Graph {
    std::unordered_map<int64_t, std::vector<int64_t>> adj_out;
    std::unordered_map<int64_t, std::vector<int64_t>> adj_in;
};

// Шаг 5: Построение графа из сегментов
Graph build_graph(const std::vector<Segment>& segments) {
    Graph graph;

    for (const auto& seg : segments) {
        graph.adj_out[seg.start_id].push_back(seg.end_id);
        graph.adj_in[seg.end_id].push_back(seg.start_id);
    }

    return graph;
}

// Проверка корректности графа: in_degree == out_degree
void validate_graph(const Graph& graph) {
    std::unordered_map<int64_t, int> in_degree, out_degree;

    for (const auto& [node, targets] : graph.adj_out) {
        out_degree[node] = static_cast<int>(targets.size());
    }

    for (const auto& [node, sources] : graph.adj_in) {
        in_degree[node] = static_cast<int>(sources.size());
    }

    // Проверяем все узлы
    for (const auto& [node, _] : graph.adj_out) {
        int in = in_degree[node];
        int out = out_degree[node];

        if (in != out) {
            std::cerr << "ОШИБКА: Узел " << node << " имеет in_degree=" << in
                      << ", out_degree=" << out << std::endl;
            throw std::runtime_error("Граф некорректен: in_degree != out_degree");
        }
    }

    for (const auto& [node, _] : graph.adj_in) {
        if (graph.adj_out.find(node) == graph.adj_out.end()) {
            int in = in_degree[node];
            std::cerr << "ОШИБКА: Узел " << node << " имеет in_degree=" << in
                      << ", out_degree=0" << std::endl;
            throw std::runtime_error("Граф некорректен: in_degree != out_degree");
        }
    }
}

// Нормализация угла в диапазон [-π, π]
double normalize_angle(double angle) {
    while (angle > M_PI) angle -= 2.0 * M_PI;
    while (angle <= -M_PI) angle += 2.0 * M_PI;
    return angle;
}

// Поиск первого угла по часовой стрелке относительно reference
// Часовая стрелка = уменьшение угла (движение от reference вниз)
int64_t find_clockwise_next(
    double reference_angle,
    const std::vector<int64_t>& candidates,
    const std::unordered_map<int64_t, double>& angles
) {
    int64_t best = -1;
    double best_angle = 0.0;
    double min_delta = 1e9;

    for (int64_t cand : candidates) {
        double cand_angle = angles.at(cand);

        // Разница углов (по часовой стрелке = отрицательная разница)
        double delta = normalize_angle(reference_angle - cand_angle);

        // Ищем минимальную положительную разницу (первое по часовой)
        if (delta > 0 && delta < min_delta) {
            min_delta = delta;
            best = cand;
            best_angle = cand_angle;
        }
    }

    // Если не нашли (все углы больше reference), берём максимальный угол
    if (best == -1) {
        double max_angle = -1e9;
        for (int64_t cand : candidates) {
            double cand_angle = angles.at(cand);
            if (cand_angle > max_angle) {
                max_angle = cand_angle;
                best = cand;
            }
        }
    }

    return best;
}

// Шаг 6: Разрешение одной седловой точки
void resolve_saddle(
    int64_t node,
    Graph& graph,
    const std::unordered_map<int64_t, Point>& point_cache
) {
    const std::vector<int64_t>& incoming = graph.adj_in[node];
    const std::vector<int64_t>& outgoing = graph.adj_out[node];

    Point V = point_cache.at(node);

    // Вычисляем углы всех входящих рёбер
    std::unordered_map<int64_t, double> in_angles;
    for (int64_t U : incoming) {
        Point U_pt = point_cache.at(U);
        double angle = std::atan2(V.y - U_pt.y, V.x - U_pt.x);
        in_angles[U] = angle;
    }

    // Вычисляем углы всех исходящих рёбер
    std::unordered_map<int64_t, double> out_angles;
    for (int64_t W : outgoing) {
        Point W_pt = point_cache.at(W);
        double angle = std::atan2(W_pt.y - V.y, W_pt.x - V.x);
        out_angles[W] = angle;
    }

    // Для каждого входящего ребра находим соответствующее исходящее
    std::vector<int64_t> new_out;
    std::vector<int64_t> used_outgoing;

    for (int64_t U : incoming) {
        double alpha = in_angles[U];

        // Ищем первое исходящее ребро по часовой стрелке от alpha
        int64_t W = find_clockwise_next(alpha, outgoing, out_angles);

        new_out.push_back(W);
        used_outgoing.push_back(W);
    }

    // Проверяем, что все исходящие рёбра использованы ровно один раз
    std::vector<int64_t> sorted_out = outgoing;
    std::vector<int64_t> sorted_used = used_outgoing;
    std::sort(sorted_out.begin(), sorted_out.end());
    std::sort(sorted_used.begin(), sorted_used.end());

    if (sorted_out != sorted_used) {
        std::cerr << "ОШИБКА: При разрешении седловой точки " << node
                  << " не все исходящие рёбра использованы корректно" << std::endl;
        throw std::runtime_error("Ошибка разрешения седловой точки");
    }

    // Заменяем исходящие рёбра на новые
    graph.adj_out[node] = new_out;
}

// Разрешение всех седловых точек
int solve_all_saddles(
    Graph& graph,
    const std::unordered_map<int64_t, Point>& point_cache
) {
    int saddle_count = 0;

    for (const auto& [node, targets] : graph.adj_out) {
        int degree = static_cast<int>(targets.size());

        if (degree > 1) {
            saddle_count++;
            resolve_saddle(node, graph, point_cache);
        }
    }

    return saddle_count;
}

// Шаг 7: Сборка полилайнов обходом циклов
std::vector<std::vector<Point>> collect_polylines(
    Graph& graph,
    const std::unordered_map<int64_t, Point>& point_cache
) {
    std::vector<std::vector<Point>> polylines;
    std::unordered_map<int64_t, std::unordered_map<int64_t, bool>> visited;

    // Инициализация visited
    for (const auto& [start, targets] : graph.adj_out) {
        for (int64_t end : targets) {
            visited[start][end] = false;
        }
    }

    // Обход всех рёбер
    for (auto& [start, targets] : graph.adj_out) {
        for (size_t i = 0; i < targets.size(); ++i) {
            int64_t current = start;
            int64_t next = targets[i];

            if (visited[current][next]) {
                continue;
            }

            // Начинаем новый цикл
            std::vector<Point> polyline;
            int64_t cycle_start = current;

            while (!visited[current][next]) {
                visited[current][next] = true;
                polyline.push_back(point_cache.at(current));

                current = next;

                // Следующее ребро (степень = 1, всегда единственное)
                if (graph.adj_out[current].empty()) {
                    std::cerr << "ОШИБКА: Узел " << current << " не имеет исходящих рёбер" << std::endl;
                    throw std::runtime_error("Граф разорван");
                }

                next = graph.adj_out[current][0];
            }

            // Замыкаем полилайн
            polyline.push_back(point_cache.at(cycle_start));

            polylines.push_back(polyline);
        }
    }

    return polylines;
}

// Проверка, что граф полностью обработан
void validate_complete_traversal(
    const Graph& graph,
    int expected_polylines
) {
    // Проверяем что все узлы имеют степень 1 или 0
    for (const auto& [node, targets] : graph.adj_out) {
        int degree = static_cast<int>(targets.size());
        if (degree != 1 && degree != 0) {
            std::cerr << "ОШИБКА: После разрешения седловых точек узел " << node
                      << " имеет степень " << degree << std::endl;
            throw std::runtime_error("Граф не полностью разрешён");
        }
    }
}

// Сохранение изолиний в файл
void save_isolines(
    const std::string& path,
    const std::vector<std::vector<Point>>& polylines
) {
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile) {
        std::cerr << "Ошибка создания файла " << path << std::endl;
        return;
    }

    // Заголовок: количество полилайнов
    int32_t num_polylines = static_cast<int32_t>(polylines.size());
    outFile.write(reinterpret_cast<const char*>(&num_polylines), sizeof(int32_t));

    // Для каждого полилайна
    for (const auto& polyline : polylines) {
        int32_t num_points = static_cast<int32_t>(polyline.size());
        outFile.write(reinterpret_cast<const char*>(&num_points), sizeof(int32_t));

        for (const auto& pt : polyline) {
            outFile.write(reinterpret_cast<const char*>(&pt.x), sizeof(double));
            outFile.write(reinterpret_cast<const char*>(&pt.y), sizeof(double));
        }
    }

    outFile.close();
    std::cout << "Изолинии сохранены в " << path << std::endl;
}

int main(int argc, char* argv[]) {
    // Проверка аргументов
    std::string input_file = "test_data.bin";
    if (argc > 1) {
        input_file = argv[1];
    }

    // Чтение test_data.bin
    std::ifstream inFile(input_file, std::ios::binary);
    if (!inFile) {
        std::cerr << "Ошибка открытия файла: " << input_file << std::endl;
        std::cerr << "Использование: " << argv[0] << " [test_data.bin]" << std::endl;
        return 1;
    }

    // Заголовок
    int32_t N, M;
    inFile.read(reinterpret_cast<char*>(&N), sizeof(int32_t));
    inFile.read(reinterpret_cast<char*>(&M), sizeof(int32_t));

    // Вершины
    std::vector<Point> vertices(N);
    for (int32_t i = 0; i < N; ++i) {
        inFile.read(reinterpret_cast<char*>(&vertices[i].x), sizeof(double));
        inFile.read(reinterpret_cast<char*>(&vertices[i].y), sizeof(double));
    }

    // Высоты
    std::vector<double> zValues(N);
    inFile.read(reinterpret_cast<char*>(zValues.data()), N * sizeof(double));

    // Треугольники
    std::vector<Triangle> triangles(M);
    inFile.read(reinterpret_cast<char*>(triangles.data()), M * sizeof(Triangle));

    // z_plane
    double z_plane;
    inFile.read(reinterpret_cast<char*>(&z_plane), sizeof(double));

    inFile.close();

    std::cout << "=== Вычисление изолиний ===" << std::endl;
    std::cout << "Вершин: " << N << std::endl;
    std::cout << "Треугольников: " << M << std::endl;
    std::cout << "Уровень сечения z_plane: " << z_plane << std::endl;
    std::cout << std::endl;

    // Шаг 1: Бинаризация
    std::vector<int8_t> state = binarize(zValues, z_plane);

    int above = 0, below = 0;
    for (auto s : state) {
        if (s == 1) above++;
        else below++;
    }
    std::cout << "Бинаризация: вершин выше=" << above << ", ниже=" << below << std::endl;

    // Шаг 3: Генерация сегментов
    std::vector<Segment> segments = generate_segments(triangles, state, vertices, zValues, z_plane);

    std::cout << "Сегментов до фильтрации: " << segments.size() << std::endl;

    // Шаг 4: Фильтрация
    int loops_removed = 0;
    int pairs_annihilated = 0;
    std::vector<Segment> filtered = filter_segments(segments, loops_removed, pairs_annihilated);

    std::cout << "Удалено петель: " << loops_removed << std::endl;
    std::cout << "Аннигилировано пар: " << pairs_annihilated << std::endl;
    std::cout << "Сегментов после фильтрации: " << filtered.size() << std::endl;
    std::cout << "Уникальных точек пересечения: " << point_cache.size() << std::endl;
    std::cout << std::endl;

    // Сохранение результата варианта B
    save_segments("segments.bin", filtered);

    std::cout << std::endl;
    std::cout << "=== Вариант C: Построение изолиний ===" << std::endl;

    try {
        // Шаг 5: Построение графа и проверка
        Graph graph = build_graph(filtered);
        std::cout << "Граф построен из " << filtered.size() << " сегментов" << std::endl;

        validate_graph(graph);
        std::cout << "Проверка графа пройдена: in_degree == out_degree для всех узлов" << std::endl;

        // Шаг 6: Разрешение седловых точек
        int saddle_count = solve_all_saddles(graph, point_cache);
        std::cout << "Седловых точек разрешено: " << saddle_count << std::endl;

        // Повторная проверка после разрешения
        validate_complete_traversal(graph, 0);
        std::cout << "Все седловые точки успешно разрешены (степени узлов = 1)" << std::endl;

        // Шаг 7: Сборка полилайнов
        std::vector<std::vector<Point>> isolines = collect_polylines(graph, point_cache);
        std::cout << "Замкнутых изолиний: " << isolines.size() << std::endl;

        // Подсчёт общего числа точек
        int total_points = 0;
        for (const auto& polyline : isolines) {
            total_points += static_cast<int>(polyline.size());
        }
        std::cout << "Общее число точек в полилайнах: " << total_points << std::endl;

        // Сохранение изолиний
        save_isolines("isolines.bin", isolines);

        std::cout << std::endl;
        std::cout << "Готово! Результаты: segments.bin, isolines.bin" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << std::endl;
        std::cerr << "КРИТИЧЕСКАЯ ОШИБКА: " << e.what() << std::endl;
        std::cerr << "Вычисления прерваны" << std::endl;
        return 1;
    }

    return 0;
}
