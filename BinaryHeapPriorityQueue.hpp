#pragma once
#include "PriorityQueueBase.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

class BinaryHeapPriorityQueue : public IPriorityQueue {
private:
    // kopiec jest zapisany w wektorze
    // dzieci elementu i sa pod indeksami 2*i+1 oraz 2*i+2
    std::vector<PQElement> heap;

    // porownanie priorytetow w jednym miejscu ulatwia czytanie kodu
    bool hasHigherPriority(const PQElement& left, const PQElement& right) const {
        return left.priority > right.priority;
    }

    // przesuwanie elementu w gore kopca
    // uzywamy tego po dodaniu lub po zwiekszeniu priorytetu
    void siftUp(std::size_t index) {
        while (index > 0) {
            // rodzic elementu w kopcu tablicowym
            std::size_t parent = (index - 1) / 2;

            // jesli rodzic ma dobry priorytet, kopiec jest juz poprawny
            if (!hasHigherPriority(heap[index], heap[parent])) {
                break;
            }

            // jesli dziecko ma wiekszy priorytet, zamieniamy je z rodzicem
            std::swap(heap[index], heap[parent]);
            index = parent;
        }
    }

    // przesuwanie elementu w dol kopca
    // uzywamy tego po usunieciu korzenia lub po zmniejszeniu priorytetu
    void siftDown(std::size_t index) {
        while (true) {
            // obliczamy indeksy lewego i prawego dziecka
            std::size_t left = 2 * index + 1;
            std::size_t right = 2 * index + 2;
            std::size_t best = index;

            // sprawdzamy, czy lewe dziecko powinno byc wyzej
            if (left < heap.size() && hasHigherPriority(heap[left], heap[best])) {
                best = left;
            }
            // sprawdzamy, czy prawe dziecko powinno byc wyzej
            if (right < heap.size() && hasHigherPriority(heap[right], heap[best])) {
                best = right;
            }
            // jesli najlepszy nadal jest obecny element, konczymy
            if (best == index) {
                break;
            }

            // zamieniamy element z lepszym dzieckiem i idziemy nizej
            std::swap(heap[index], heap[best]);
            index = best;
        }
    }

public:
    std::string name() const override {
        return "Kopiec_binarny";
    }

    void push(int id, int value, int priority) override {
        // nowy element dodajemy na koniec tablicy
        heap.push_back({id, value, priority});

        // potem przesuwamy go w gore, az kopiec bedzie poprawny
        siftUp(heap.size() - 1);
    }

    PQElement pop() override {
        if (heap.empty()) {
            throw std::runtime_error("Kolejka jest pusta");
        }

        // w kopcu maksimum jest zawsze na poczatku
        PQElement result = heap.front();

        // ostatni element przenosimy na poczatek
        heap.front() = heap.back();
        heap.pop_back();

        // po takiej zmianie trzeba naprawic kopiec od gory
        if (!heap.empty()) {
            siftDown(0);
        }
        return result;
    }

    const PQElement& peek() const override {
        if (heap.empty()) {
            throw std::runtime_error("Kolejka jest pusta");
        }
        // peek jest szybki, bo najlepszy element jest w korzeniu
        return heap.front();
    }

    bool changePriority(int id, int newPriority) override {
        // w zwyklym kopcu nie mamy mapy id na indeks, wiec szukamy liniowo
        for (std::size_t i = 0; i < heap.size(); ++i) {
            if (heap[i].id == id) {
                // zapisujemy stary priorytet, zeby wiedziec jak naprawiac kopiec
                int oldPriority = heap[i].priority;
                heap[i].priority = newPriority;

                // gdy priorytet wzrosl, element moze isc do gory
                if (newPriority > oldPriority) {
                    siftUp(i);
                } else {
                    // gdy priorytet zmalal, element moze isc w dol
                    siftDown(i);
                }
                return true;
            }
        }
        // nie znaleziono elementu o takim id
        return false;
    }

    std::size_t size() const override {
        // rozmiar kopca to rozmiar wektora
        return heap.size();
    }

    void clear() override {
        // usuniecie wszystkich elementow
        heap.clear();
    }

    void loadElements(const std::vector<PQElement>& data) override {
        // kopiujemy dane i budujemy z nich kopiec
        heap = data;
        heap.reserve(heap.size() + 1);
        if (heap.empty()) {
            return;
        }

        // naprawiamy kopiec od ostatniego rodzica do korzenia
        for (int i = static_cast<int>(heap.size() / 2) - 1; i >= 0; --i) {
            siftDown(static_cast<std::size_t>(i));
        }
    }
};
