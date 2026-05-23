# Intersection Calculator

C++ консольное приложение для вычисления пересечений и изолиний на триангулированных поверхностях.

## Компоненты проекта

### Основное приложение
- `intersectionCalculator.cpp` - основное приложение (в разработке)

### Генератор тестовых данных
- `generate_test_data_cdt.cpp` - генератор триангулированной поверхности с тестовыми данными
- `read_test_data.cpp` - утилита для проверки и чтения бинарных данных

## Установка зависимостей

### CDT библиотека

Генератор использует библиотеку [CDT](https://github.com/artem-ogre/CDT) для быстрой триангуляции Делоне.

**Автоматическая установка (рекомендуется):**

CDT уже клонирована в `external/CDT` и собрана. Если её нет:

```bash
# Клонирование
cd external
git clone https://github.com/artem-ogre/CDT.git

# Сборка через CMake
cd CDT/CDT
mkdir build && cd build
cmake -DCDT_USE_AS_COMPILED_LIBRARY=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

После этого `external/CDT/CDT/build/Release/CDT.lib` готова к использованию.

## Сборка и запуск

### 1. Полный генератор тестовых данных (50k вершин)

**Требования:**
- MSVC компилятор (Visual Studio)
- CDT библиотека собрана (см. выше)
- CMake 3.8+

**Компиляция (через Visual Studio Command Prompt):**
```cmd
cl /I.\external\CDT\CDT\include /I.\external\CDT\CDT\build /EHsc /O2 /MD /std:c++17 /nologo generate_test_data_cdt.cpp /link external\CDT\CDT\build\Release\CDT.lib /OUT:generate_test_data_cdt.exe
```

**Запуск:**
```powershell
.\generate_test_data_cdt.exe
```

Создаёт:
- `test_data.bin` (~2.5 МБ) - полные данные триангуляции
- `test_data_report.txt` - статистика

**Время генерации:** ~10 секунд

### 2. Простой генератор (9 вершин, для быстрого тестирования)

**Компиляция:**
```cmd
cl /EHsc /O2 /std:c++17 /nologo generate_simple_test.cpp /Fe:generate_simple_test.exe
```

**Запуск:**
```powershell
.\generate_simple_test.exe
```

**Время генерации:** <1 секунда

### 3. Основное приложение (вычисление изолиний)

**Компиляция:**
```cmd
cl /EHsc /O2 /std:c++17 /nologo intersectionCalculator.cpp /Fe:intersectionCalculator.exe
```

**Запуск:**
```powershell
.\intersectionCalculator.exe test_data.bin
```

Создаёт `segments.bin` с результатами.

### 4. Утилиты просмотра

**Проверка test_data.bin:**
```cmd
cl /EHsc /O2 /std:c++17 /nologo read_test_data.cpp /Fe:read_test_data.exe
.\read_test_data.exe
```

**Просмотр segments.bin:**
```cmd
cl /EHsc /O2 /std:c++17 /nologo view_segments.cpp /Fe:view_segments.exe
.\view_segments.exe
```

## Формат test_data.bin

Бинарный файл содержит триангулированную поверхность:

```
Offset  | Type          | Описание
--------|---------------|----------------------------------
0       | int32         | N - количество вершин
4       | int32         | M - количество треугольников
8       | double[N*2]   | Координаты вершин (x,y)
...     | double[N]     | Высоты z
...     | int32[M*3]    | Индексы треугольников (CCW)
...     | double        | z_plane - уровень сечения
...     | uint8[N]      | Граничная маска
...     | uint8[N]      | Маска плато
...     | uint8[N]      | Маска хребта
...     | int32         | Индекс изолированного пика (-1 если нет)
```

## Характеристики тестовых данных

- **Количество вершин:** ~50,500 (50,000 внутренних + граница)
- **Количество треугольников:** ~100,000
- **Область:** квадрат [-100, 100]²
- **Вырез:** пятиконечная звезда в центре
- **Рельеф:** синусоидальная функция с особыми зонами:
  - Граничные вершины: z = 1000
  - Плато (кольцо r ∈ [35, 45]): z = 0
  - Хребет (x ∈ [-80, -60], y ≈ 80): z = 0
  - Изолированный пик: z = 0 в специальной точке
- **Уровень сечения:** z_plane = 0.0
- **Ориентация треугольников:** против часовой стрелки (CCW)

## Структура проекта

```
intersectionCalculator/
├── intersectionCalculator.cpp      # Основное приложение (вариант B)
├── intersectionCalculator.vcxproj  # Проект Visual Studio
├── intersectionCalculator.slnx     # Решение Visual Studio
├── generate_test_data_cdt.cpp      # Полный генератор (CDT)
├── generate_simple_test.cpp        # Простой генератор (быстрый)
├── read_test_data.cpp              # Утилита проверки test_data.bin
├── view_segments.cpp               # Утилита просмотра segments.bin
├── external/
│   └── CDT/                        # Библиотека триангуляции
│       └── CDT/
│           ├── include/            # Заголовочные файлы
│           ├── src/                # Исходники
│           └── build/              # Собранная библиотека
│               └── Release/
│                   └── CDT.lib
├── CLAUDE.md                       # Инструкции для Claude Code
├── README.md                       # Этот файл
├── GENERATOR_NOTES.md              # Техническая документация генератора
└── CALCULATOR_NOTES.md             # Техническая документация алгоритма
```

## Сборка через Visual Studio

Откройте `intersectionCalculator.slnx` в Visual Studio 2022+ и нажмите F7 для сборки.

**Платформы:** Win32, x64  
**Конфигурации:** Debug, Release  
**Стандарт:** C++20

## Требования

- **ОС:** Windows 10/11
- **Компилятор:** MSVC (Visual Studio 2022+ или Build Tools)
- **CMake:** 3.8+ (для сборки CDT)
- **C++ стандарт:** C++17 (генератор и утилиты), C++20 (основное приложение)

## Результаты работы

При обработке полных тестовых данных (50k вершин):
```
Вершин: 50,570
Треугольников: 100,570
Сегментов до фильтрации: 5,646
Удалено петель: 314
Аннигилировано пар: 1
Сегментов после фильтрации: 5,330
Уникальных точек пересечения: 5,332
```

Файлы результатов:
- `segments.bin` (~208 КБ) - ориентированные сегменты изолиний
- Формат описан в `CALCULATOR_NOTES.md`
