#include <iostream>
#include <vector>
#include <list>
#include <stxxl/vector>
#include <stxxl/algorithm>
#include <stxxl/bits/mempool.h> // Necesario para stxxl::simple_pool

// Definiciones de configuración del caché
const size_t CACHE_SIZE = 1024; // Tamaño total del caché en elementos
const size_t BLOCK_SIZE = 32;   // Tamaño del bloque en elementos
//const size_t NUM_WAYS = 4;    // Asociatividad del caché (cambiar para 2, 4 u 8)
const size_t NUM_ELEMENTS = 2048; // Número total de elementos a acceder

// Plantilla para la clase Cache, parametrizada por la asociatividad
template <size_t NUM_WAYS>
class Cache {
public:
    // Constructor
    Cache() : hits(0), misses(0) {
        // Inicializa el caché.  Usa una lista de listas.
        // Cada lista interna representa un conjunto.
        cache.resize(CACHE_SIZE / (BLOCK_SIZE * NUM_WAYS));
    }

    // Función para acceder al caché
    bool access(size_t address) {
        size_t block_address = address / BLOCK_SIZE; // Calcula la dirección del bloque
        size_t set_index = block_address % (CACHE_SIZE / (BLOCK_SIZE * NUM_WAYS)); // Calcula el índice del conjunto

        // Busca el bloque en el conjunto correspondiente
        for (auto it = cache[set_index].begin(); it != cache[set_index].end(); ++it) {
            if (*it == block_address) {
                // ¡Hit! Mueve el bloque al frente de la lista (LRU)
                cache[set_index].erase(it);
                cache[set_index].push_front(block_address);
                hits++;
                return true;
            }
        }

        // Miss.  Trae el bloque a la caché
        if (cache[set_index].size() == NUM_WAYS) {
            // El conjunto está lleno, expulsa el bloque menos recientemente usado (LRU)
            cache[set_index].pop_back();
        }
        cache[set_index].push_front(block_address); // Agrega el nuevo bloque al frente
        misses++;
        return false;
    }

    // Función para obtener el número de aciertos
    size_t getHits() const { return hits; }

    // Función para obtener el número de fallos
    size_t getMisses() const { return misses; }

private:
    std::vector<std::list<size_t>> cache; // Estructura de datos del caché
    size_t hits;     // Contador de aciertos
    size_t misses;   // Contador de fallos
};

int main() {
    // Ejemplo de uso con un caché asociativo de 4 vías
    std::cout << "Ejemplo de Cache Asociativo de 2, 4 y 8 Vias con STXXL" << std::endl;

    for (int num_ways : {2, 4, 8}) {
      std::cout << "\nEjecutando con " << num_ways << " vias:" << std::endl;
      if (num_ways == 2) {
        Cache<2> cache;
        // Simula una secuencia de accesos a memoria
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            cache.access(i);
        }

        // Imprime las estadísticas del caché
        std::cout << "Hits: " << cache.getHits() << std::endl;
        std::cout << "Misses: " << cache.getMisses() << std::endl;
        std::cout << "Hit Rate: " << (double)cache.getHits() / (cache.getHits() + cache.getMisses()) << std::endl;
      }
      else if (num_ways == 4)
      {
        Cache<4> cache;
        // Simula una secuencia de accesos a memoria
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            cache.access(i);
        }

        // Imprime las estadísticas del caché
        std::cout << "Hits: " << cache.getHits() << std::endl;
        std::cout << "Misses: " << cache.getMisses() << std::endl;
        std::cout << "Hit Rate: " << (double)cache.getHits() / (cache.getHits() + cache.getMisses()) << std::endl;
      }
      else
      {
        Cache<8> cache;
        // Simula una secuencia de accesos a memoria
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            cache.access(i);
        }

        // Imprime las estadísticas del caché
        std::cout << "Hits: " << cache.getHits() << std::endl;
        std::cout << "Misses: " << cache.getMisses() << std::endl;
        std::cout << "Hit Rate: " << (double)cache.getHits() / (cache.getHits() + cache.getMisses()) << std::endl;
      }
    }
    return 0;
}

