#pragma once

#include "PriorityQueueBase.hpp"

#include <algorithm>
#include <string>
#include <vector>

class SortedArrayPriorityQueue : public IPriorityQueue {
private:
    // elementy sa stale ulozone rosnaco po priorytecie
    // najwyzszy priorytet jest na koncu wektora
    std::vector<PQElement> elements;

    // szukamy miejsca, gdzie trzeba wstawic nowy element
    // przechodzimy od poczatku do momentu, gdzie priorytet juz nie pasuje
    std::size_t positionFor(int priority) const {
        std::size_t left = 0;
        std::size_t right = elements.size();

        // szukanie binarne szybciej znajduje miejsce w posortowanej tablicy
        while (left < right) {
            std::size_t middle = left + (right - left) / 2;
            if (elements[middle].priority <= priority) {
                left = middle + 1;
            } else {
                right = middle;
            }
        }
        return left;
    }

    // wspolna metoda do wstawiania elementu w dobre miejsce
    // uzywamy jej przy dodawaniu i przy zmianie priorytetu
    void insertElement(const PQElement& element) {
        std::size_t position = positionFor(element.priority);
        elements.insert(elements.begin() + static_cast<long long>(position), element);
    }

public:
    std::string name() const override {
        return "Tablica_posortowana";
    }

    void push(int id, int value, int priority) override {
        // dodanie musi zachowac porzadek, dlatego wstawiamy w srodek
        insertElement({id, value, priority});
    }

    PQElement pop() override {
        if (elements.empty()) {
            throw std::runtime_error("Kolejka jest pusta");
        }

        // najlepszy element jest na koncu, wiec usuniecie jest szybkie
        PQElement result = elements.back();
        elements.pop_back();
        return result;
    }

    const PQElement& peek() const override {
        if (elements.empty()) {
            throw std::runtime_error("Kolejka jest pusta");
        }
        // podejrzenie tez jest szybkie, bo najwyzszy priorytet jest na koncu
        return elements.back();
    }

    bool changePriority(int id, int newPriority) override {
        // najpierw trzeba znalezc element o podanym id
        for (std::size_t i = 0; i < elements.size(); ++i) {
            if (elements[i].id == id) {
                // kopiujemy element, bo po zmianie priorytetu moze miec inne miejsce
                PQElement updated = elements[i];
                updated.priority = newPriority;

                // stary element usuwamy z obecnej pozycji
                elements.erase(elements.begin() + static_cast<long long>(i));

                // potem wstawiamy go ponownie tak, zeby tablica byla posortowana
                insertElement(updated);
                return true;
            }
        }
        // jesli nie ma elementu o takim id, zwracamy false
        return false;
    }

    std::size_t size() const override {
        // rozmiar pobieramy bezposrednio z wektora
        return elements.size();
    }

    void clear() override {
        // czyscimy cala tablice
        elements.clear();
    }

    void loadElements(const std::vector<PQElement>& data) override {
        // kopiujemy dane i sortujemy je rosnaco po priorytecie
        elements = data;
        elements.reserve(elements.size() + 1);
        std::sort(elements.begin(), elements.end(), [](const PQElement& a, const PQElement& b) {
            return a.priority < b.priority;
        });
    }
};
