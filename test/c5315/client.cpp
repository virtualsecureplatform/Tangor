#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>
#include <memory>
#include <random>
#include <tfhe++.hpp>
#include <vector>

int main()
{
    // generate a random key
    std::unique_ptr<TFHEpp::SecretKey> sk(new TFHEpp::SecretKey);
    TFHEpp::EvalKey ek;
    ek.emplacebkfft<TFHEpp::lvl01param>(*sk);
    #ifdef USE_HOGE
    ek.emplacebkntt<TFHEpp::lvl01param>(*sk);
    #endif
    ek.emplaceiksk<TFHEpp::lvl10param>(*sk);

    // export the secret key to file for later use
    {
        std::ofstream ofs{"secret.key", std::ios::binary};
        cereal::PortableBinaryOutputArchive ar(ofs);
        sk->serialize(ar);
    };

    // export the cloud key to a file (for the cloud)
    {
        std::ofstream ofs{"eval.key", std::ios::binary};
        cereal::PortableBinaryOutputArchive ar(ofs);
        ek.serialize(ar);
    };

    // generate encrypt the input
    constexpr uint bitwidth = 178;
    std::random_device seed_gen;
    std::default_random_engine engine(seed_gen());

    std::vector<uint8_t> p(bitwidth);
    std::uniform_int_distribution<uint64_t> inrand(0, (1 << bitwidth) - 1);
    std::array<uint64_t,bitwidth/64 + 1> ina = {};
    for (int i = 0; i < bitwidth/64; i++) ina[i] = inrand(engine);
    for (int i = 0; i < bitwidth/64; i++) for(int j = 0; j < 64; j++) p[i*64+j] = (ina[i] >> j) & 1;

    std::vector<TFHEpp::TLWE<TFHEpp::lvl1param>> ciphertext =
        TFHEpp::bootsSymEncrypt(p, *sk);

    // export the 2+2*bitwith ciphertexts to a file (for the cloud)
    {
        std::ofstream ofs{"cloud.data", std::ios::binary};
        cereal::PortableBinaryOutputArchive ar(ofs);
        ar(ciphertext);
    };
}