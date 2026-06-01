#include "BinaryHeapPriorityQueue.hpp"
#include "SortedArrayPriorityQueue.hpp"
#include "UnsortedArrayPriorityQueue.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
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
// potem takie rekordy zapisujemy do csv
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

// zapis wszystkich wynikow do pliku csv
void saveCsv(const std::vector<BenchmarkResult>& results, const std::string& path) {
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

    // zapis wynikow dopliku wyniki.csv
    saveCsv(results, "wyniki.csv");

    std::cout << "\nGotowe.\n";
    std::cout << "Plik zapisany bezposrednio jako: wyniki.csv\n";
    std::cout << "Suma kontrolna: " << antiOptimization << '\n';

    return 0;
}