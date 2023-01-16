#include <wrappedcodelets.hpp>
#include <starpu_build_graph.hpp>
#include <starpu_bound.h>
#include <fstream>
#include <iostream>
#include <stdio.h>

using namespace Tangor;

int main (int argc, char* argv[]){


    // To see performance
    std::chrono::system_clock::time_point start, init, end;
    start = std::chrono::system_clock::now();

    // reads the cloud key from file
    {
        const std::string path = "./eval.key";
        std::ifstream ifs("./eval.key", std::ios::in | std::ios::binary);
        cereal::PortableBinaryInputArchive ar(ifs);
        ek.serialize(ar);
    }

    // Read input ciphertexts
    {
        std::ifstream ifs("./cloud.data", std::ios::binary);
        cereal::PortableBinaryInputArchive ar(ifs);
        ar(cipherin);
    }

	// Parse JSON netlist
    std::ifstream ifs("./circuit.json", std::ios::in);
    if (ifs.fail()) {
        std::cerr << "failed to read json file" << std::endl;
        return 1;
    }
    const std::string json((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
    ifs.close();
    const YosysJSONparser::ParsedBC BCnetlist(json);

    starpu_init(NULL);


#ifdef USE_HOGE
    std::cout<<"Initializing HOGE"<<std::endl;
    HOGEinit(argv[1],ek);
#endif

    starpu_bound_start(1,0);

	starpu_build_graph(BCnetlist,ek);
    std::cout<<"Start"<<std::endl;
    init = std::chrono::system_clock::now();

	starpu_task_wait_for_all();

    end = std::chrono::system_clock::now();
    starpu_bound_stop();

    // export the result ciphertexts to a file
    {
        std::ofstream ofs{"./result.data", std::ios::binary};
        cereal::PortableBinaryOutputArchive ar(ofs);
        ar(cipherout);
    }

    {
        FILE *f = fopen("minimum_runtime.lp", "w");
        starpu_bound_print_lp(f);
        fclose(f);
    }

    starpu_shutdown();

    double time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(init - start)
            .count() /
        1000.0);
    printf("init time %lf[ms]\n", time);

    time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - init)
            .count() /
        1000.0);
    printf("run time %lf[ms]\n", time);

    time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count() /
        1000.0);
    printf("total time %lf[ms]\n", time);

    {
        #ifdef USE_HOGE
        std::ofstream ofs("HOGE_runtime.txt",std::ios::app);
        #else
        std::ofstream ofs("CPU_runtime.txt",std::ios::app);
        #endif
        ofs<<static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - init)
            .count() /
        1000.0)<<std::endl;
    }
}
