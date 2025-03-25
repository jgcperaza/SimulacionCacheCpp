#include <iostream>
#include <vector>
#include <list>
#include <random>
#include <stxxl/vector>
#include <stxxl/io>

// Definiciones de configuración del caché
const size_t CACHE_SIZE = 1024; // Tamaño total del caché en elementos
const size_t My_BLOCK_SIZE = 32;   // Tamaño del bloque en elemento
//const size_t NUM_WAYS = 4;    // Asociatividad del caché (cambiar para 2, 4 u 8)
const size_t NUM_ELEMENTS = 2048; // Número total de elementos a acceder

const size_t PREFETCH_DISTANCE = 2;

struct Block {
    int data[My_BLOCK_SIZE]; // Ejemplo: bloque de 32 elementos

};

// Plantilla para la clase Cache, parametrizada por la asociatividad
template <size_t NUM_WAYS>
class Cache {
public:
    // Constructor
    Cache() : hits(0), misses(0) {
        // Inicializa el caché.  Usa una lista de listas.
        // Cada lista interna representa un conjunto.
        cache.resize(CACHE_SIZE / (My_BLOCK_SIZE * NUM_WAYS));
        external_blocks.resize(NUM_ELEMENTS / My_BLOCK_SIZE);

        for (size_t i = 0; i < external_blocks.size(); ++i) {
            external_blocks[i].data[0] = static_cast<int>(i); // Datos dummy
        }
    }

    // Función para acceder al caché
    bool access(size_t address) {
        size_t block_address = address / My_BLOCK_SIZE; // Calcula la dirección del bloque
        size_t set_index = block_address % (CACHE_SIZE / (My_BLOCK_SIZE * NUM_WAYS)); // Calcula el índice del conjunto

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

        Block block = external_blocks[block_address];
         // Miss: Precargar bloques cercanos
         prefetch_adjacent_blocks(block_address);
         auto& set = cache[set_index];

         if (set.size() == NUM_WAYS) {
             set.pop_back(); // Expulsar LRU
         }
         set.push_front(block_address);
 
        
         misses++;
         return false;
     }
 
     void prefetch_adjacent_blocks(size_t block_address) {
         for (size_t i = block_address + 1; i <= block_address + PREFETCH_DISTANCE; ++i) {
             if (i < external_blocks.size() && !is_block_in_cache(i)) {
                 // Simular carga anticipada
                 [[maybe_unused]] auto volatile dummy = external_blocks[i].data[0];
             }
         }
    }

    bool is_block_in_cache(size_t block_address) const {
        size_t set_index = block_address % cache.size();
        const auto& set = cache[set_index];
        return std::find(set.begin(), set.end(), block_address) != set.end();
    }
    // Función para obtener el número de aciertos
    size_t getHits() const { return hits; }

    // Función para obtener el número de fallos
    size_t getMisses() const { return misses; }

private:
    stxxl::vector<Block> external_blocks; 
    std::vector<std::list<size_t>> cache; // Estructura de datos del caché
    size_t hits;     // Contador de aciertos
    size_t misses;   // Contador de fallos
};

int main() {
    // Ejemplo de uso con un caché asociativo de 4 vías
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, NUM_ELEMENTS/4);

    std::cout << "Ejemplo de Cache Asociativo de 2, 4 y 8 Vias con STXXL" << std::endl;

    for (int num_ways : {2, 4, 8}) {
      std::cout << "\nEjecutando con " << num_ways << " vias:" << std::endl;
      if (num_ways == 2) {
        Cache<2> cache;
        // Simula una secuencia de accesos a memoria
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            size_t address = dist(gen) + (i % 4) * (NUM_ELEMENTS/4); // Dirección aleatoria
            cache.access(address);
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
            size_t address = dist(gen) + (i % 4) * (NUM_ELEMENTS/4);// Dirección aleatoria
            cache.access(address);
            
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
            size_t address = dist(gen) + (i % 4) * (NUM_ELEMENTS/4); // Dirección aleatoria
            cache.access(address);

        }

        // Imprime las estadísticas del caché
        std::cout << "Hits: " << cache.getHits() << std::endl;
        std::cout << "Misses: " << cache.getMisses() << std::endl;
        std::cout << "Hit Rate: " << (double)cache.getHits() / (cache.getHits() + cache.getMisses()) << std::endl;
      }
    }
    return 0;
}
     