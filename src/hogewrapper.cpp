#include <hogewrapper.hpp>
#include <string>
namespace Tangor{


#define PC_NAME(n) n | XCL_MEM_TOPOLOGY
constexpr std::array <int, 32> pc = {
    PC_NAME(0),  PC_NAME(1),  PC_NAME(2),  PC_NAME(3),  PC_NAME(4),  PC_NAME(5),  PC_NAME(6),  PC_NAME(7),
    PC_NAME(8),  PC_NAME(9),  PC_NAME(10), PC_NAME(11), PC_NAME(12), PC_NAME(13), PC_NAME(14), PC_NAME(15),
    PC_NAME(16), PC_NAME(17), PC_NAME(18), PC_NAME(19), PC_NAME(20), PC_NAME(21), PC_NAME(22), PC_NAME(23),
    PC_NAME(24), PC_NAME(25), PC_NAME(26), PC_NAME(27), PC_NAME(28), PC_NAME(29), PC_NAME(30), PC_NAME(31)};

cl_mem_ext_ptr_t inaBuf, inbBuf, resBuf;
alignas(4096) TFHEpp::TLWE<TFHEpp::lvl1param> buftlwea,buftlweb,bufres;
std::unique_ptr<cl::Buffer> buffer_res,buffer_ina,buffer_inb;
std::array<cl_mem_ext_ptr_t, iksknumbus> ikskBufs;
std::array<cl_mem_ext_ptr_t, bknumbus> bknttBufs;

std::unique_ptr<cl::Context> context;
std::unique_ptr<cl::CommandQueue> q;
std::unique_ptr<cl::Kernel> kernel;
cl::Program* program;

std::array<std::shared_ptr<cl::Buffer>, bknumbus> buffer_bkntts;
std::array<std::shared_ptr<cl::Buffer>, iksknumbus> buffer_iksks;

void HOGEinit(const std::string& binaryFile, const TFHEpp::EvalKey& ek){
    std::cout << binaryFile << std::endl;
    cl_int err;
	// The get_xil_devices will return vector of Xilinx Devices
    auto devices = xcl::get_xil_devices();

    // read_binary_file() command will find the OpenCL binary file created using
    // the
    // V++ compiler load into OpenCL Binary and return pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);

    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = std::make_unique<cl::Context>(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = std::make_unique<cl::CommandQueue>(*context, device, CL_QUEUE_PROFILING_ENABLE, &err));

        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        program = new cl::Program(*context, {device}, bins, nullptr, &err);
        // program = std::make_unique<cl::Program>(*context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, kernel = std::make_unique<cl::Kernel>(*program, "HomGate", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    //allgned to distribute to module
	constexpr uint buswidthlb = 9;
	constexpr uint buswords = 1U<<(buswidthlb-6);
	constexpr uint iksknumbus = 10;
	constexpr uint totaliksknumbus = 40;
	constexpr uint bknumbus = 8;
	constexpr uint cyclebit = 5;
	constexpr uint numcycle = 1<<cyclebit;
	constexpr uint wordsinbus = (1U<<buswidthlb)/std::numeric_limits<typename TFHEpp::lvl0param::T>::digits;

  	alignas(4096) std::array<std::array<std::array<std::array<std::array<std::array<typename TFHEpp::lvl0param::T, wordsinbus>, totaliksknumbus/iksknumbus>, (1 << TFHEpp::lvl10param::basebit) - 1>, TFHEpp::lvl10param::t>,TFHEpp::lvl1param::n>,iksknumbus> ikskaligned = {};
    for(int i = 0; i<TFHEpp::lvl1param::n; i++) for(int j = 0; j < TFHEpp::lvl10param::t; j++) for(int k = 0; k< (1 << TFHEpp::lvl10param::basebit) - 1; k++) for(int l = 0; l < wordsinbus; l++) for(int m = 0; m < iksknumbus; m++) for(int n = 0; n < totaliksknumbus/iksknumbus; n++) ikskaligned[m][i][j][k][n][l] = (ek.getiksk<TFHEpp::lvl10param>())[i][j][k][n*iksknumbus*wordsinbus+m*wordsinbus+l];
    alignas(4096) std::array<std::array<std::array<std::array<std::array<uint64_t,buswords>,numcycle>,2*TFHEpp::lvl1param::l>,TFHEpp::lvl0param::n>,bknumbus> bknttaligned = {};
    for(int k =0; k < 2; k++) for(int bus = 0; bus < bknumbus/2; bus++) for(int i = 0; i < TFHEpp::lvl0param::n; i++) for(int l = 0; l < 2*TFHEpp::lvl1param::l; l++) for(int cycle = 0; cycle < numcycle; cycle++) for(int word = 0; word<buswords; word++) bknttaligned[k*bknumbus/2+bus][i][l][cycle][word] = (ek.getbkntt<TFHEpp::lvl01param>())[i][l][k][cycle*bknumbus/2*buswords+bus*buswords+word].value;

	resBuf.param = 0;
	resBuf.flags = PC_NAME(0);
    resBuf.obj = bufres.data();

	inaBuf.param = 0;
	inaBuf.flags = PC_NAME(1);
    inaBuf.obj = buftlwea.data();

	inbBuf.param = 0;
	inbBuf.flags = PC_NAME(2);
    inbBuf.obj = buftlweb.data();

    for(int i = 0; i < iksknumbus; i++){
        ikskBufs[i].obj = ikskaligned[i].data();
        ikskBufs[i].param = 0;
        ikskBufs[i].flags = pc[i+3];
    }

    for(int i = 0; i < bknumbus; i++){
        bknttBufs[i].obj = bknttaligned[i].data();
        bknttBufs[i].param = 0;
        bknttBufs[i].flags = pc[20+i];
    }

    for(int i = 0; i < iksknumbus; i++){
        buffer_iksks[i] = std::make_shared<cl::Buffer>(*context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                                sizeof(ikskaligned[i]), &ikskBufs[i], &err);
    }
    for(int i = 0; i < bknumbus; i++){
        buffer_bkntts[i] = std::make_shared<cl::Buffer>(*context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                                sizeof(bknttaligned[i]), &bknttBufs[i], &err);
    }
    OCL_CHECK(err, buffer_ina = std::make_unique<cl::Buffer>(*context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                        sizeof(buftlwea), &inaBuf, &err));
    OCL_CHECK(err, buffer_inb = std::make_unique<cl::Buffer>(*context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                            sizeof(buftlweb), &inbBuf, &err));
    OCL_CHECK(err, buffer_res = std::make_unique<cl::Buffer>(*context, CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                            sizeof(bufres), &resBuf, &err));

    // Setting the kernel Arguments
    OCL_CHECK(err, err = (*kernel).setArg(3, *buffer_res));
    OCL_CHECK(err, err = (*kernel).setArg(4, *buffer_ina));
    OCL_CHECK(err, err = (*kernel).setArg(5, *buffer_inb));
    for(int i = 0; i < iksknumbus; i++) OCL_CHECK(err, err = (*kernel).setArg(i+6, *(buffer_iksks[i])));
    for(int i = 0; i < bknumbus; i++) OCL_CHECK(err, err = (*kernel).setArg(i+6+iksknumbus, *(buffer_bkntts[i])));

    // Copy EvalKey to Device Global Memory
    std::vector<cl::Memory> inputs(iksknumbus+bknumbus);
    for(int i = 0; i < iksknumbus; i++) inputs[i] = *(buffer_iksks[i]);
    for(int i = 0; i < bknumbus; i++) inputs[i+iksknumbus] = *(buffer_bkntts[i]);
    OCL_CHECK(err, err = q->enqueueMigrateMemObjects(inputs, 0 /* 0 means from host*/));
    q->finish();
}

}
