#include "BinaryHeapPriorityQueue.hpp"
#include "SortedArrayPriorityQueue.hpp"
#include "UnsortedArrayPriorityQueue.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

// ta zmienna chroni przed tym, zeby kompilator nie wyrzucil pomiarow
// wynik operacji jest do niej dodawany, wiec kod faktycznie sie wykonuje
volatile long long antiOptimization = 0;

// jeden rekord z wynikiem pojedynczego pomiaru
// potem takie rekordy zapisujemy do csv i raportu
struct BenchmarkResult {
    std::string structure;
    std::string operation;
    int n;
    long long repetitions;
    long long totalNs;

    // sredni czas jednej operacji w nanosekundach
    double averageNs() const {
        return static_cast<double>(totalNs) / static_cast<double>(repetitions);
    }
};

// fabryka kolejek
// dla podanego numeru tworzy odpowiednia implementacje
std::unique_ptr<IPriorityQueue> createQueue(int type) {
    if (type == 0) {
        return std::make_unique<UnsortedArrayPriorityQueue>();
    }
    if (type == 1) {
        return std::make_unique<SortedArrayPriorityQueue>();
    }
    return std::make_unique<BinaryHeapPriorityQueue>();
}

// mala funkcja pomocnicza do testow poprawnosci
// jesli warunek nie jest spelniony, program konczy sie bledem
void requireCondition(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Blad testu poprawnosci: " + message);
    }
}

// proste testy sprawdzajace, czy kazda kolejka dziala tak samo
// nie sa to pelne testy jednostkowe, ale wykrywaja najwazniejsze bledy
void runCorrectnessTests() {
    for (int type = 0; type < 3; ++type) {
        // tworzymy kolejke danego typu
        auto queue = createQueue(type);

        // dodajemy trzy elementy z roznymi priorytetami
        queue->push(1, 100, 10);
        queue->push(2, 200, 5);
        queue->push(3, 300, 20);

        // sprawdzamy podstawowe operacje
        requireCondition(queue->size() == 3, queue->name() + " - zly rozmiar po dodaniu");
        requireCondition(queue->peek().id == 3, queue->name() + " - zly element po peek");

        // element z id 2 dostaje najwyzszy priorytet
        requireCondition(queue->changePriority(2, 30), queue->name() + " - brak zmiany priorytetu");
        requireCondition(queue->peek().id == 2, queue->name() + " - zly peek po zmianie priorytetu");

        // usuwanie powinno isc od najwiekszego priorytetu
        requireCondition(queue->pop().id == 2, queue->name() + " - zly pierwszy pop");
        requireCondition(queue->pop().id == 3, queue->name() + " - zly drugi pop");
        requireCondition(queue->pop().id == 1, queue->name() + " - zly trzeci pop");
        requireCondition(queue->empty(), queue->name() + " - kolejka nie jest pusta");
    }
}

// tworzy dane testowe o podanym rozmiarze
// priorytety rosna, zeby dane byly proste i powtarzalne
std::vector<PQElement> makeData(int n) {
    std::vector<PQElement> data;
    data.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        data.push_back({i, i * 3, i + 1});
    }

    return data;
}

// szybkie przygotowanie kolejki o rozmiarze n
// tego przygotowania nie wliczamy do czasu operacji
void prepareQueue(IPriorityQueue& queue, int n) {
    std::vector<PQElement> data = makeData(n);
    queue.loadElements(data);
}

// zamienia roznice dwoch punktow czasu na nanosekundy
long long elapsedNs(const Clock::time_point& start, const Clock::time_point& end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

// liczba powtorzen dla peek zalezy od rozmiaru
// dzieki temu bardzo wolne przypadki nie trwaja zbyt dlugo
int checksForLinearOperation(int n) {
    return std::max(5, std::min(1000, 1000000 / n));
}

// pomiar dodania jednego elementu do kolejki o rozmiarze n
BenchmarkResult benchmarkPush(int queueType, int n, int rounds) {
    long long totalNs = 0;
    long long operations = 0;

    for (int round = 0; round < rounds; ++round) {
        auto queue = createQueue(queueType);
        prepareQueue(*queue, n);

        // mierzymy tylko jedno dodanie do gotowej kolejki
        auto start = Clock::now();
        queue->push(n + round, (n + round) * 3, n + round + 1);
        auto end = Clock::now();

        antiOptimization += static_cast<long long>(queue->size());
        totalNs += elapsedNs(start, end);
        operations += 1;
    }

    return {createQueue(queueType)->name(), "dodawanie", n, operations, totalNs};
}

// pomiar usuniecia jednego elementu z kolejki o rozmiarze n
BenchmarkResult benchmarkPop(int queueType, int n, int rounds) {
    long long totalNs = 0;
    long long operations = 0;

    for (int round = 0; round < rounds; ++round) {
        auto queue = createQueue(queueType);
        prepareQueue(*queue, n);

        // mierzymy tylko jedno usuniecie najlepszego elementu
        auto start = Clock::now();
        PQElement element = queue->pop();
        auto end = Clock::now();

        antiOptimization += element.priority;
        totalNs += elapsedNs(start, end);
        operations += 1;
    }

    return {createQueue(queueType)->name(), "usuwanie", n, operations, totalNs};
}

// pomiar zwracania rozmiaru kolejki
// ta operacja jest bardzo szybka, wiec powtarzamy ja wiele razy
BenchmarkResult benchmarkSize(int queueType, int n, int rounds) {
    const int checks = 200000;
    long long totalNs = 0;
    long long operations = 0;

    for (int round = 0; round < rounds; ++round) {
        auto queue = createQueue(queueType);
        prepareQueue(*queue, n);

        // mierzymy wiele odczytow rozmiaru jednej kolejki
        auto start = Clock::now();
        for (int i = 0; i < checks; ++i) {
            antiOptimization += static_cast<long long>(queue->size());
        }
        auto end = Clock::now();

        totalNs += elapsedNs(start, end);
        operations += checks;
    }

    return {createQueue(queueType)->name(), "rozmiar", n, operations, totalNs};
}

// pomiar peek na kolejce o rozmiarze n
// peek nie usuwa elementu, wiec mozna go powtarzac na tej samej kolejce
BenchmarkResult benchmarkPeek(int queueType, int n, int rounds) {
    const int checks = checksForLinearOperation(n);
    long long totalNs = 0;
    long long operations = 0;

    for (int round = 0; round < rounds; ++round) {
        auto queue = createQueue(queueType);
        prepareQueue(*queue, n);

        // mierzymy samo podgladanie najlepszego elementu
        auto start = Clock::now();
        for (int i = 0; i < checks; ++i) {
            const PQElement& element = queue->peek();
            antiOptimization += element.priority;
        }
        auto end = Clock::now();

        totalNs += elapsedNs(start, end);
        operations += checks;
    }

    return {createQueue(queueType)->name(), "peek", n, operations, totalNs};
}

// pomiar zmiany priorytetu jednego elementu w kolejce o rozmiarze n
BenchmarkResult benchmarkChangePriority(int queueType, int n, int rounds) {
    long long totalNs = 0;
    long long operations = 0;

    for (int round = 0; round < rounds; ++round) {
        auto queue = createQueue(queueType);
        prepareQueue(*queue, n);

        // wybieramy element ze srodka, zeby nie byl zawsze na poczatku ani koncu
        int id = n / 2;
        int newPriority = n * 3 + round;

        // mierzymy tylko jedna zmiane priorytetu
        auto start = Clock::now();
        bool changed = queue->changePriority(id, newPriority);
        auto end = Clock::now();

        if (changed) {
            antiOptimization += newPriority;
        }
        totalNs += elapsedNs(start, end);
        operations += 1;
    }

    return {createQueue(queueType)->name(), "zmiana_priorytetu", n, operations, totalNs};
}

// formatowanie liczby do dwoch miejsc po przecinku
std::string formatDouble(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

// zapis wszystkich wynikow do pliku csv
// taki plik mozna potem otworzyc np w excelu
void saveCsv(const std::vector<BenchmarkResult>& results, const std::filesystem::path& path) {
    std::ofstream file(path);

    // pierwszy wiersz to naglowki kolumn
    file << "struktura,operacja,n,liczba_prob,czas_calkowity_ns,sredni_czas_ns\n";

    // kazdy wynik zapisujemy jako jeden wiersz
    for (const BenchmarkResult& result : results) {
        file << result.structure << ','
             << result.operation << ','
             << result.n << ','
             << result.repetitions << ','
             << result.totalNs << ','
             << std::fixed << std::setprecision(2) << result.averageNs() << '\n';
    }
}

// wyszukuje wynik dla danej struktury, operacji i rozmiaru
const BenchmarkResult* findResult(const std::vector<BenchmarkResult>& results,
                                  const std::string& structure,
                                  const std::string& operation,
                                  int n) {
    for (const BenchmarkResult& result : results) {
        if (result.structure == structure && result.operation == operation && result.n == n) {
            return &result;
        }
    }
    return nullptr;
}

// zapis tabel do raportu markdown
void saveReport(const std::vector<BenchmarkResult>& results,
                const std::vector<int>& sizes,
                const std::filesystem::path& path,
                int rounds) {
    std::ofstream file(path);

    // raport jest w markdown, bo latwo go czytac i przerobic na pdf
    file << "# Porownanie kolejek priorytetowych\n\n";
    file << "Projekt mierzy piec operacji dla trzech implementacji kolejki priorytetowej: "
         << "tablicy nieposortowanej, tablicy posortowanej i kopca binarnego.\n\n";
    file << "Rozmiary kolejek: 1000, 2000, 5000, 10000, 20000, 50000, 100000.\n";
    file << "Dla kazdego rozmiaru kolejka jest najpierw przygotowana, a potem mierzona jest jedna operacja. "
         << "Sredni czas liczony jest na podstawie " << rounds << " prob.\n\n";
    file << "Oczekiwana zlozonosc operacji:\n\n";
    file << "| Struktura | Dodawanie | Usuwanie | Rozmiar | Peek | Zmiana priorytetu |\n";
    file << "|---|---:|---:|---:|---:|---:|\n";
    file << "| Tablica nieposortowana | O(1) | O(n) | O(1) | O(n) | O(n) |\n";
    file << "| Tablica posortowana | O(n) | O(1) | O(1) | O(1) | O(n) |\n";
    file << "| Kopiec binarny | O(log n) | O(log n) | O(1) | O(1) | O(n) |\n\n";

    std::vector<std::string> operations = {
        "dodawanie", "usuwanie", "rozmiar", "peek", "zmiana_priorytetu"
    };
    std::vector<std::string> structures = {
        "Tablica_nieposortowana", "Tablica_posortowana", "Kopiec_binarny"
    };

    for (const std::string& operation : operations) {
        file << "## " << operation << "\n\n";
        file << "| n | Tablica nieposortowana [ns] | Tablica posortowana [ns] | Kopiec binarny [ns] |\n";
        file << "|---:|---:|---:|---:|\n";

        for (int n : sizes) {
            file << "| " << n;
            for (const std::string& structure : structures) {
                const BenchmarkResult* result = findResult(results, structure, operation, n);
                file << " | ";
                if (result != nullptr) {
                    file << formatDouble(result->averageNs());
                } else {
                    file << "-";
                }
            }
            file << " |\n";
        }
        file << "\n";
    }

    file << "Wykresy liniowo-punktowe znajduja sie w pliku `wykresy.svg`.\n";
}

// zapis krotkiego podsumowania do csv
void saveSummaryCsv(const std::vector<BenchmarkResult>& results,
                    const std::vector<int>& sizes,
                    const std::filesystem::path& path) {
    std::ofstream file(path);

    // ten plik pokazuje najlepsza strukture dla kazdej operacji i kazdego n
    file << "operacja,n,najszybsza_struktura,najszybszy_sredni_czas_ns\n";

    std::vector<std::string> operations = {
        "dodawanie", "usuwanie", "rozmiar", "peek", "zmiana_priorytetu"
    };

    for (const std::string& operation : operations) {
        for (int n : sizes) {
            const BenchmarkResult* best = nullptr;
            for (const BenchmarkResult& result : results) {
                if (result.operation == operation && result.n == n) {
                    if (best == nullptr || result.averageNs() < best->averageNs()) {
                        best = &result;
                    }
                }
            }

            if (best != nullptr) {
                file << operation << ','
                     << n << ','
                     << best->structure << ','
                     << formatDouble(best->averageNs()) << '\n';
            }
        }
    }
}

// skrocone nazwy struktur, zeby miescily sie na wykresie
std::string shortName(const std::string& structure) {
    if (structure == "Tablica_nieposortowana") {
        return "tn";
    }
    if (structure == "Tablica_posortowana") {
        return "tp";
    }
    return "kb";
}

// pelna nazwa struktury do legendy
std::string displayName(const std::string& structure) {
    if (structure == "Tablica_nieposortowana") {
        return "tablica nieposortowana";
    }
    if (structure == "Tablica_posortowana") {
        return "tablica posortowana";
    }
    return "kopiec binarny";
}

// skala osi y z malym zapasem u gory
double chartMaxValue(const std::vector<BenchmarkResult>& results, const std::string& operation) {
    double maxValue = 1.0;
    for (const BenchmarkResult& result : results) {
        if (result.operation == operation) {
            maxValue = std::max(maxValue, result.averageNs());
        }
    }
    return maxValue * 1.10;
}

// pozycja punktu na osi x
// rozmiary sa rozlozone rowno, a podpis pokazuje prawdziwe n
double xForIndex(int left, int plotWidth, std::size_t index, std::size_t count) {
    if (count <= 1) {
        return static_cast<double>(left);
    }
    return static_cast<double>(left) + (static_cast<double>(plotWidth) * index) / static_cast<double>(count - 1);
}

// pozycja punktu na osi y
double yForValue(int top, int plotHeight, double value, double maxValue) {
    return static_cast<double>(top) + static_cast<double>(plotHeight)
         - (value / maxValue) * static_cast<double>(plotHeight);
}

// generuje wykresy liniowo-punktowe w jednym pliku svg
void saveSvgCharts(const std::vector<BenchmarkResult>& results,
                   const std::vector<int>& sizes,
                   const std::filesystem::path& path) {
    std::vector<std::string> operations = {
        "dodawanie", "usuwanie", "rozmiar", "peek", "zmiana_priorytetu"
    };
    std::vector<std::string> structures = {
        "Tablica_nieposortowana", "Tablica_posortowana", "Kopiec_binarny"
    };
    std::vector<std::string> colors = {"#2f80ed", "#27ae60", "#eb5757"};

    const int width = 1200;
    const int chartHeight = 360;
    const int totalHeight = 90 + static_cast<int>(operations.size()) * chartHeight;
    std::ofstream svg(path);

    // poczatek pliku svg i podstawowe style tekstu
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
        << "\" height=\"" << totalHeight << "\" viewBox=\"0 0 " << width
        << ' ' << totalHeight << "\">\n";
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    svg << "<style>text{font-family:Arial,sans-serif;font-size:13px}.title{font-size:22px;font-weight:bold}.small{font-size:11px}</style>\n";
    svg << "<text x=\"30\" y=\"38\" class=\"title\">porownanie kolejek priorytetowych</text>\n";
    svg << "<text x=\"30\" y=\"62\">wykresy liniowo-punktowe: os x to rozmiar kolejki, os y to sredni czas operacji w ns</text>\n";

    for (std::size_t opIndex = 0; opIndex < operations.size(); ++opIndex) {
        // dla kazdej operacji tworzymy osobny wykres
        const std::string& operation = operations[opIndex];
        int top = 110 + static_cast<int>(opIndex) * chartHeight;
        int left = 95;
        int plotWidth = 960;
        int plotHeight = 230;
        int base = top + plotHeight;
        double maxValue = chartMaxValue(results, operation);

        // tytul i osie
        svg << "<text x=\"30\" y=\"" << (top - 28) << "\" font-weight=\"bold\">" << operation << "</text>\n";
        svg << "<line x1=\"" << left << "\" y1=\"" << base << "\" x2=\"" << (left + plotWidth)
            << "\" y2=\"" << base << "\" stroke=\"#333\"/>\n";
        svg << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left
            << "\" y2=\"" << base << "\" stroke=\"#333\"/>\n";
        svg << "<text x=\"22\" y=\"" << (top + 5) << "\" class=\"small\">" << formatDouble(maxValue) << " ns</text>\n";
        svg << "<text x=\"30\" y=\"" << (base + 36) << "\" class=\"small\">rozmiar kolejki</text>\n";

        // podpisy osi x
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            double x = xForIndex(left, plotWidth, i, sizes.size());
            svg << "<line x1=\"" << x << "\" y1=\"" << base << "\" x2=\"" << x
                << "\" y2=\"" << (base + 5) << "\" stroke=\"#333\"/>\n";
            svg << "<text x=\"" << x << "\" y=\"" << (base + 20)
                << "\" text-anchor=\"middle\" class=\"small\">" << sizes[i] << "</text>\n";
        }

        // legenda
        for (std::size_t s = 0; s < structures.size(); ++s) {
            int legendX = 850;
            int legendY = top - 32 + static_cast<int>(s) * 18;
            svg << "<line x1=\"" << legendX << "\" y1=\"" << legendY
                << "\" x2=\"" << (legendX + 26) << "\" y2=\"" << legendY
                << "\" stroke=\"" << colors[s] << "\" stroke-width=\"3\"/>\n";
            svg << "<circle cx=\"" << (legendX + 13) << "\" cy=\"" << legendY
                << "\" r=\"4\" fill=\"" << colors[s] << "\"/>\n";
            svg << "<text x=\"" << (legendX + 35) << "\" y=\"" << (legendY + 4)
                << "\" class=\"small\">" << displayName(structures[s]) << "</text>\n";
        }

        // linie i punkty dla kazdej implementacji
        for (std::size_t s = 0; s < structures.size(); ++s) {
            std::vector<std::pair<double, double>> points;

            for (std::size_t i = 0; i < sizes.size(); ++i) {
                const BenchmarkResult* result = findResult(results, structures[s], operation, sizes[i]);
                if (result == nullptr) {
                    continue;
                }

                double x = xForIndex(left, plotWidth, i, sizes.size());
                double y = yForValue(top, plotHeight, result->averageNs(), maxValue);
                points.push_back({x, y});
            }

            if (points.empty()) {
                continue;
            }

            svg << "<polyline fill=\"none\" stroke=\"" << colors[s]
                << "\" stroke-width=\"2.5\" points=\"";
            for (const auto& point : points) {
                svg << point.first << "," << point.second << " ";
            }
            svg << "\"/>\n";

            for (std::size_t i = 0; i < points.size(); ++i) {
                svg << "<circle cx=\"" << points[i].first << "\" cy=\"" << points[i].second
                    << "\" r=\"4\" fill=\"" << colors[s] << "\"/>\n";
                svg << "<text x=\"" << points[i].first << "\" y=\"" << (points[i].second - 8)
                    << "\" text-anchor=\"middle\" class=\"small\">" << shortName(structures[s]) << "</text>\n";
            }
        }
    }

    // koniec pliku svg
    svg << "</svg>\n";
}

int main() {
    // rozmiary danych uzywane w pomiarach
    std::vector<int> sizes = {1000, 2000, 5000, 10000, 20000, 50000, 100000};

    // mamy trzy implementacje kolejki
    const int queueTypes = 3;

    // liczba prob taka jak w projekcie pierwszym
    const int rounds = 50;

    // tutaj zbieramy wszystkie wyniki benchmarkow
    std::vector<BenchmarkResult> results;

    // najpierw sprawdzamy poprawnosc, potem dopiero mierzymy czas
    runCorrectnessTests();
    std::cout << "Testy poprawnosci zakonczone sukcesem.\n";
    std::cout << "Start pomiarow kolejek priorytetowych...\n";

    // petle przechodza po wszystkich strukturach i rozmiarach
    for (int type = 0; type < queueTypes; ++type) {
        for (int n : sizes) {
            // kazda z wymaganych operacji ma osobny pomiar
            results.push_back(benchmarkPush(type, n, rounds));
            results.push_back(benchmarkPop(type, n, rounds));
            results.push_back(benchmarkSize(type, n, rounds));
            results.push_back(benchmarkPeek(type, n, rounds));
            results.push_back(benchmarkChangePriority(type, n, rounds));

            std::cout << "Zakonczono: " << createQueue(type)->name()
                      << ", n=" << n << '\n';
        }
    }

    // tworzymy katalog na pliki wynikowe
    std::filesystem::create_directory("wyniki");

    // zapis wynikow w kilku formatach
    saveCsv(results, "wyniki/wyniki.csv");
    saveSummaryCsv(results, sizes, "wyniki/podsumowanie.csv");
    saveReport(results, sizes, "wyniki/raport.md", rounds);
    saveSvgCharts(results, sizes, "wyniki/wykresy.svg");

    std::cout << "\nGotowe.\n";
    std::cout << "Pliki zapisane w katalogu: wyniki\n";
    std::cout << "Suma kontrolna: " << antiOptimization << '\n';

    return 0;
}
