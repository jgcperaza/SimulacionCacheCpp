#include <iostream>
#include <vector>
#include <list>
#include <random>
#include <memory>
#include <iomanip>
#include <chrono>
#include <stxxl/vector>

// ===================== CONFIGURACIÓN =====================
const size_t CACHE_SIZE = 2048;      // Tamaño de caché en elementos
const size_t MyBLOCK_SIZE = 32;        // Tamaño de bloque en elementos
const size_t NUM_ELEMENTS = 4096;    // Número total de accesos a simular
const size_t PREFETCH_DISTANCE = 8;  // Bloques a precargar
const double HOT_ACCESS_PROB = 0.5;  // Probabilidad de acceder a bloques calientes

// ===================== ESTRUCTURA DE BLOQUE =====================
class Block {
public:
    int data[MyBLOCK_SIZE];
    
    Block() {
        std::fill_n(data, MyBLOCK_SIZE, 0);
    }
    
    explicit Block(int initial_value) {
        for (size_t i = 0; i < MyBLOCK_SIZE; ++i) {
            data[i] = initial_value + static_cast<int>(i);
        }
    }
};

// ===================== MEMORIA EXTERNA (STXXL) =====================
class ExternalMemory {
public:
    ExternalMemory(size_t num_blocks) : blocks(num_blocks) {
        // Inicialización de bloques en memoria externa
        for (size_t i = 0; i < num_blocks; ++i) {
            blocks[i] = Block(static_cast<int>(i * MyBLOCK_SIZE));
        }
    }
    
    const Block& read_block(size_t block_address) const {
        if (block_address >= blocks.size()) {
            throw std::out_of_range("Dirección de bloque inválida");
        }
        return blocks[block_address];
    }
    
    size_t size() const { return blocks.size(); }

private:
    stxxl::vector<Block> blocks;
};

// ===================== CONJUNTO DE CACHÉ =====================
class CacheSet {
public:
    CacheSet(size_t num_ways) : ways(num_ways) {}
    
    // Acceso a un bloque en el conjunto (política LRU)
    bool access(size_t block_address) {
        auto it = std::find(blocks.begin(), blocks.end(), block_address);
        
        if (it != blocks.end()) {
            // Hit: Mover al frente (LRU)
            blocks.erase(it);
            blocks.push_front(block_address);
            return true;
        }
        
        // Miss: Insertar nuevo bloque
        if (blocks.size() >= ways) {
            blocks.pop_back(); // Eliminar el menos reciente si está lleno
        }
        blocks.push_front(block_address);
        return false;
    }
    
    bool contains(size_t block_address) const {
        return std::find(blocks.begin(), blocks.end(), block_address) != blocks.end();
    }

private:
    std::list<size_t> blocks; // Lista de bloques en este conjunto
    size_t ways;              // Número de vías (asociatividad)
};

// ===================== CACHÉ COMPLETA =====================
template <size_t NUM_WAYS>
class Cache {
public:
    Cache(std::shared_ptr<ExternalMemory> memory) 
        : memory(memory), hits(0), misses(0), prefetch_hits(0) {
        size_t num_sets = CACHE_SIZE / (MyBLOCK_SIZE * NUM_WAYS);
        if (num_sets == 0) num_sets = 1; // Mínimo 1 conjunto
        sets.resize(num_sets, CacheSet(NUM_WAYS));
    }
    
    // Acceso a una dirección de memoria
    bool access(size_t address) {
        size_t block_address = address / MyBLOCK_SIZE;
        size_t set_index = block_address % sets.size();
        
        if (sets[set_index].access(block_address)) {
            hits++;
            return true;
        }
        
        // Miss: Precargar bloques adyacentes
        prefetch_adjacent_blocks(block_address);
        misses++;
        return false;
    }
    
    // Precarga de bloques adyacentes
    void prefetch_adjacent_blocks(size_t block_address) {
        for (size_t i = block_address + 1; i <= block_address + PREFETCH_DISTANCE; ++i) {
            if (i < memory->size()) {
                if (is_block_in_cache(i)) {
                    prefetch_hits++;
                } else {
                    // Precargar el bloque aunque no lo usemos inmediatamente
                    [[maybe_unused]] auto volatile dummy = memory->read_block(i);
                }
            }
        }
    }
    
    bool is_block_in_cache(size_t block_address) const {
        size_t set_index = block_address % sets.size();
        return sets[set_index].contains(block_address);
    }
    
    // Métricas de rendimiento
    size_t getHits() const { return hits; }
    size_t getMisses() const { return misses; }
    size_t getPrefetchHits() const { return prefetch_hits; }
    double getHitRate() const {
        return static_cast<double>(hits) / (hits + misses);
    }
    double getEffectiveHitRate() const {
        return static_cast<double>(hits + prefetch_hits) / (hits + misses + prefetch_hits);
    }

private:
    std::shared_ptr<ExternalMemory> memory;
    std::vector<CacheSet> sets; // Todos los conjuntos de la caché
    size_t hits;
    size_t misses;
    size_t prefetch_hits;
};

// ===================== PATRONES DE ACCESO =====================
class MemoryAccessPattern {
public:
    virtual ~MemoryAccessPattern() = default;
    virtual size_t next_address() = 0;
    virtual std::string name() const = 0;
};

// Patrón de acceso realista (80% accesos a bloques calientes)
class RealisticAccessPattern : public MemoryAccessPattern {
public:
    RealisticAccessPattern(size_t max_blocks, size_t num_elements)
        : gen(std::random_device{}()),
          hot_block_dist(0, max_blocks/4-1),  // 25% bloques "calientes"
          cold_block_dist(0, max_blocks-1),
          element_dist(0, MyBLOCK_SIZE-1),
          count(0),
          num_elements(num_elements) {}
    
    size_t next_address() override {
        if (count >= num_elements) return 0;
        
        bool access_hot = std::uniform_real_distribution<>(0.0, 1.0)(gen) < HOT_ACCESS_PROB;
        size_t block = access_hot ? hot_block_dist(gen) : cold_block_dist(gen);
        size_t address = (block * MyBLOCK_SIZE) + element_dist(gen);
        
        count++;
        return address;
    }
    
    std::string name() const override {
        return "Patrón realista (80% accesos a bloques calientes)";
    }

private:
    std::mt19937 gen;
    std::uniform_int_distribution<size_t> hot_block_dist;
    std::uniform_int_distribution<size_t> cold_block_dist;
    std::uniform_int_distribution<size_t> element_dist;
    size_t count;
    size_t num_elements;
};

// ===================== SIMULADOR PRINCIPAL =====================
class CacheSimulator {
public:
    CacheSimulator() {
        // Configuración básica de STXXL
        std::ofstream cfg("stxxl_config.txt");
        cfg << "disk=stxxl.tmp,200MiB,syscall delete_on_exit\n";
        cfg.close();
        setenv("STXXLCFG", "stxxl_config.txt", 1);
    }
    
    ~CacheSimulator() {
        std::remove("stxxl_config.txt");
    }
    
    template <size_t NUM_WAYS>
    void run_simulation(const std::string& name) {
        // Preparación
        auto memory = std::make_shared<ExternalMemory>(NUM_ELEMENTS / MyBLOCK_SIZE);
        RealisticAccessPattern pattern(memory->size(), NUM_ELEMENTS);
        Cache<NUM_WAYS> cache(memory);
        
        std::cout << "\nSimulando caché " << name << "-vías con " << pattern.name() << "\n";
        
        // Ejecución
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            cache.access(pattern.next_address());
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end_time - start_time;
        
        // Resultados
        print_results(name, cache, duration);
    }

private:
    void print_results(const std::string& name, const auto& cache, 
                      const std::chrono::duration<double>& duration) {
        std::cout << "\nRESULTADOS " << name << "-VÍAS\n";
        std::cout << "================================\n";
        std::cout << "Configuración:\n";
        std::cout << "- Tamaño caché: " << CACHE_SIZE << " elementos\n";
        std::cout << "- Tamaño bloque: " << MyBLOCK_SIZE << " elementos\n";
        std::cout << "- Asociatividad: " << name << " vías\n";
        std::cout << "- Prefetch: " << PREFETCH_DISTANCE << " bloques\n";
        std::cout << "- Elementos totales: " << NUM_ELEMENTS << "\n";
        std::cout << "\nMétricas:\n";
        std::cout << "- Tiempo simulación: " << duration.count() << "s\n";
        std::cout << "- Aciertos (Hits): " << cache.getHits() << "\n";
        std::cout << "- Fallos (Misses): " << cache.getMisses() << "\n";
        std::cout << "- Prefetch hits: " << cache.getPrefetchHits() << "\n";
        std::cout << "- Tasa aciertos: " << std::fixed << std::setprecision(2) 
                  << (cache.getHitRate() * 100) << "%\n";
        std::cout << "- Tasa aciertos efectiva (con prefetch): " 
                  << (cache.getEffectiveHitRate() * 100) << "%\n";
        std::cout << "================================\n";
    }
};

// ===================== PROGRAMA PRINCIPAL =====================
int main() {
    std::cout << "\nSIMULADOR EDUCATIVO DE CACHÉ ASOCIATIVO CON PREFETCHING\n";
    std::cout << "======================================================\n";
    
    try {
        CacheSimulator simulator;
        
        simulator.run_simulation<2>("2");
        simulator.run_simulation<4>("4");
        simulator.run_simulation<8>("8");
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
