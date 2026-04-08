#include <GraphBLAS.h>
#include <LAGraph.h>
#include <LAGraphX.h>
#include <parser.h>
#include <sys/resource.h>
#include <time.h>

#define run_search_algo()                                                      \
    LAGraph_CFL_single_path(outputs, &pi_type, adj_matrices, grammar.terms_count,        \
                            grammar.nonterms_count, grammar.rules,             \
                            grammar.rules_count, msg)

#define run_extract_algo()                                                     \
    LAGraph_CFL_extract_single_path(                                           \
        &paths, &startv, &endv, 0, adj_matrices, outputs, grammar.terms_count, \
        grammar.nonterms_count, grammar.rules, grammar.rules_count, msg)

GrB_Matrix *adj_matrices = NULL;
GrB_Matrix *outputs = NULL;
GrB_Type pi_type = NULL;
grammar_t grammar = {0, 0, 0, NULL};
PathArray paths;
char msg[LAGRAPH_MSG_LEN];

void setup() { LAGr_Init(GrB_NONBLOCKING, malloc, NULL, NULL, free, msg); }

void teardown(void) { LAGraph_Finalize(msg); }

void init_outputs() {
    outputs = calloc(grammar.nonterms_count, sizeof(GrB_Matrix));
}

void free_outputs() {
    GrB_free(&pi_type);
    for (size_t i = 0; i < grammar.nonterms_count; i++) {
        if (outputs == NULL)
            break;

        if (outputs[i] == NULL)
            continue;

        GrB_free(&outputs[i]);
    }
    free(outputs);
    outputs = NULL;
}

void free_workspace() {

    for (size_t i = 0; i < grammar.terms_count; i++) {
        if (adj_matrices == NULL)
            break;

        if (adj_matrices[i] == NULL)
            continue;

        GrB_free(&adj_matrices[i]);
    }
    free(adj_matrices);
    adj_matrices = NULL;

    free_outputs();

    free(grammar.rules);
    grammar = (grammar_t){0, 0, 0, NULL};
}

void free_path_array() {
    if (paths.paths != NULL) {
        for (size_t i = 0; i < paths.count; i++) {
            LAGraph_Free((void **)&paths.paths[i].edges, msg);
        }
    }
    LAGraph_Free((void **)&paths.paths, msg);
    paths.capacity = 0;
    paths.count = 0;
}

// The function extracts the graph name from the config.
// For example, from “data/graphs/rdf/go_hierarchy.g,data/grammars/”,
// it will extract the string “go_hierarchy” into the param name.
char *extract_graph_name(const char *config_string, char *name,
                         size_t name_size) {
    if (!config_string || !name || name_size == 0) {
        return NULL;
    }

    char *temp = strdup(config_string);
    if (!temp) {
        return NULL;
    }

    char *comma = strchr(temp, ',');
    if (comma) {
        *comma = '\0';
    }

    char *last_slash = strrchr(temp, '/');
    char *filename;

    if (last_slash) {
        filename = last_slash + 1;
    } else {
        filename = temp;
    }

    char name_copy[256];
    strncpy(name_copy, filename, sizeof(name_copy) - 1);
    name_copy[sizeof(name_copy) - 1] = '\0';

    char *dot = strrchr(name_copy, '.');
    if (dot) {
        *dot = '\0';
    }

    strncpy(name, name_copy, name_size - 1);
    name[name_size - 1] = '\0';

    free(temp);
    return name;
}

char *configs_configs_rdf[] = {"data/graphs/rdf/go_hierarchy.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/taxonomy.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/eclass.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/go.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/taxonomy_hierarchy.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       NULL};

char *configs_java[] = {
    "data/graphs/java/eclipse.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/lusearch.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/luindex.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/commons_io.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/sunflow.g,data/grammars/java_points_to.cnf",
    NULL};

char *configs_c_alias[] = {
    "data/graphs/c_alias/init.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/block.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/fs.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/ipc.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/lib.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/mm.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/net.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/security.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/sound.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/arch.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/crypto.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/drivers.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/kernel.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/postgre.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/apache.g,data/grammars/c_alias.cnf",
    NULL};

char *configs_vf[] = {"data/graphs/vf/xz.g,data/grammars/vf.cnf",
                      "data/graphs/vf/nab.g,data/grammars/vf.cnf",
                      NULL};

char *configs_my[] = {"data/graphs/vf/xz.g,data/grammars/vf.cnf", NULL};

// Number of benchmark runs on a single graph
#define COUNT 1
// If true, the first run is done without measuring time (warm-up)
#define HOT true
// Use your custom configuration for the benchmark (default is the xz.g graph
// and vf.cnf grammar)
#define configs configs_c_alias

int main(int argc, char **argv) {
    setup();
    GrB_Info retval;

    printf("Start bench\n");
    fflush(stdout);

    // To extract path
    FILE *benchmark_file = fopen("c_alias_bench_result.csv", "w");
    if (benchmark_file == NULL) {
        perror("Failed to create result file");
        return 1;
    }
    fprintf(benchmark_file, "graph_name,path_id,time_ms\n");

    int config_index = 0;
    char *config = configs[config_index];
    while (config) {
        printf("CONFIG: %s\n", config);
        char graph_name[256];
        extract_graph_name(config, graph_name, sizeof(graph_name));
        fflush(stdout);
        parser(config, &grammar, &adj_matrices);

        double start[COUNT];
        double end[COUNT];

        if (HOT) {
            run_search_algo();
        }

        GrB_Index nnz = 0;
        for (size_t i = 0; i < COUNT; i++) {
            init_outputs();

            start[i] = LAGraph_WallClockTime();
#ifndef CI
            retval = run_search_algo();
#endif
            end[i] = LAGraph_WallClockTime();

            GrB_Matrix_nvals(&nnz, outputs[0]);
            printf("\t%.3fs", end[i] - start[i]);
            fflush(stdout);
#ifndef CI
            if (i == 0) { // extract path - single run per path
                GrB_Index *row = NULL;
                GrB_Index *col = NULL;
                LAGraph_Malloc((void **)&row, nnz, sizeof(GrB_Index), msg);
                LAGraph_Malloc((void **)&col, nnz, sizeof(GrB_Index), msg);

                GrB_Matrix_extractTuples_UDT(row, col, NULL, &nnz, outputs[i]);

                for (GrB_Index k = 0; k < nnz; k++) {
                    GrB_Index startv = row[k];
                    GrB_Index endv = col[k];

                    double start_s = LAGraph_WallClockTime();
                    run_extract_algo();
                    double end_s = LAGraph_WallClockTime();
                    double elapsed_s = end_s - start_s;
                    double elapsed_ms = elapsed_s * 1000;

                    fprintf(benchmark_file, "%s,path_%ld,%.3f\n", graph_name, k,
                            elapsed_ms);
                    fflush(benchmark_file);

                    free_path_array();
                }

                printf("Single-path extraction benchmark completed.\n"
                       "Results saved in benchmark_results.csv\n");
                LAGraph_Free((void **)&row, msg);
                LAGraph_Free((void **)&col, msg);
            }
#endif
            free_outputs();
        }
        printf("\n");

        double sum = 0;
        for (size_t i = 0; i < COUNT; i++) {
            sum += end[i] - start[i];
        }

        printf("\tTime elapsed (avg): %.6f seconds. Result: %ld (return code "
               "%d) (%s)\n\n",
               sum / COUNT, nnz, retval, msg);

        free_workspace();
        config = configs[++config_index];
        fflush(stdout);
    }

    fclose(benchmark_file);
    teardown();
}
