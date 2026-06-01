#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

// pojedynczy element przechowywany w kolejce
// id sluzy do wyszukania elementu przy zmianie priorytetu
// value to zwykla wartosc, a priority decyduje o kolejnosci
struct PQElement {
    int id;
    int value;
    int priority;
};

// wspolny interfejs dla wszystkich kolejek, dzieki temu w main mozna testowac kazda strukture tak samo
class IPriorityQueue {
public:
    // wirtualny destruktor jest potrzebny przy dziedziczeniu
    virtual ~IPriorityQueue() = default;

    // nazwa struktury uzywana potem w plikach csv i raporcie
    virtual std::string name() const = 0;

    // dodanie nowego elementu do kolejki
    virtual void push(int id, int value, int priority) = 0;

    // usuniecie elementu o najwyzszym priorytecie
    virtual PQElement pop() = 0;

    // podejrzenie elementu o najwyzszym priorytecie bez usuwania
    virtual const PQElement& peek() const = 0;

    // zmiana priorytetu elementu wyszukiwanego po id
    virtual bool changePriority(int id, int newPriority) = 0;

    // zwrocenie aktualnego rozmiaru kolejki
    virtual std::size_t size() const = 0;

    // wyczyszczenie calej kolejki
    virtual void clear() = 0;

    // szybkie przygotowanie kolejki do pomiaru
    // domyslnie dodaje elementy zwykla metoda push
    virtual void loadElements(const std::vector<PQElement>& data) {
        clear();
        for (const PQElement& element : data) {
            push(element.id, element.value, element.priority);
        }
    }

    // metoda pomocnicza, zeby latwo sprawdzic czy kolejka jest pusta
    bool empty() const {
        return size() == 0;
    }
};
