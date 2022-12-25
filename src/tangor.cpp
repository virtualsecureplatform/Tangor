#include <wrappedcodelets.hpp>
#include <starpu_build_graph.hpp>
#include <fstream>
#include <iostream>
// #include <starpu_heteroprio.h>

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

    // struct starpu_conf conf;
    // starpu_conf_init(&conf);
    // conf.sched_policy_name = "heteroprio";
    // conf.sched_policy_init = &init_heteroprio;

    // // Init StarPU
    // starpu_init(&conf);
    
    starpu_init(NULL);


#ifdef USE_HOGE
    std::cout<<"Initializing HOGE"<<std::endl;
    HOGEinit(argv[1],ek);
#endif

	starpu_build_graph(BCnetlist,ek);
    std::cout<<"Start"<<std::endl;
    init = std::chrono::system_clock::now();

	starpu_task_wait_for_all();

    starpu_shutdown();

        // export the result ciphertexts to a file
    {
        std::ofstream ofs{"./result.data", std::ios::binary};
        cereal::PortableBinaryOutputArchive ar(ofs);
        ar(cipherout);
    }

    end = std::chrono::system_clock::now();
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
}