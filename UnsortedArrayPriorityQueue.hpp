#pragma once

#include "PriorityQueueBase.hpp"

#include <string>
#include <vector>

class UnsortedArrayPriorityQueue : public IPriorityQueue {
private:
    // elementy sa trzymane zwyczajnie w wektorze
    // nie pilnujemy tutaj zadnego sortowania
    std::vector<PQElement> elements;

    // szukamy indeksu elementu z najwiekszym priorytetem
    // w tablicy nieposortowanej trzeba przejsc po wszystkich elementach
    std::size_t bestIndex() const {
        if (elements.empty()) {
            throw std::runtime_error("Kolejka jest pusta");
        }

        // na poczatku zakladamy, ze najlepszy jest pierwszy element
        std::size_t best = 0;

        // potem porownujemy go z kazdym kolejnym elementem
        for (std::size_t i = 1; i < elements.size(); ++i) {
            if (elements[i].priority > elements[best].priority) {
                // jesli znalezlismy wiekszy priorytet, zapamietujemy indeks
                best = i;
            }
        }
        return best;
    }

public:
    std::string name() const override {
        return "Tablica_nieposortowana";
    }

    void push(int id, int value, int priority) override {
        // dodawanie jest szybkie, bo tylko dopisujemy na koniec wektora
        elements.push_back({id, value, priority});
    }

    PQElement pop() override {
        // najpierw znajdujemy element o najwyzszym priorytecie
        std::size_t index = bestIndex();

        // zapamietujemy go, bo zaraz usuniemy go z tablicy
        PQElement result = elements[index];

        // wstawiamy ostatni element w miejsce usuwanego
        // dzieki temu usuniecie nie musi przesuwac calej tablicy
        elements[index] = elements.back();
        elements.pop_back();
        return result;
    }

    const PQElement& peek() const override {
        // peek tylko zwraca najlepszy element, ale go nie usuwa
        return elements[bestIndex()];
    }

    bool changePriority(int id, int newPriority) override {
        // szukamy elementu po id, bo tablica nie jest posortowana
        for (PQElement& element : elements) {
            if (element.id == id) {
                // po znalezieniu wystarczy podmienic priorytet
                element.priority = newPriority;
                return true;
            }
        }
        // false oznacza, ze nie znaleziono elementu o takim id
        return false;
    }

    std::size_t size() const override {
        // wektor sam przechowuje swoj rozmiar
        return elements.size();
    }

    void clear() override {
        // usuwamy wszystkie elementy z kolejki
        elements.clear();
    }

    void loadElements(const std::vector<PQElement>& data) override {
        // dla tablicy nieposortowanej wystarczy skopiowac gotowe dane
        elements = data;

        // zostawiamy miejsce na jedno dodanie bez realokacji
        elements.reserve(elements.size() + 1);
    }
};
